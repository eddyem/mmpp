/*
 * This file is part of the libmmpp project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tty_procs.h"
#include <errno.h>
#include <fcntl.h>          // read
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>      // ioctl
#include <sys/stat.h>       // read
#include <sys/time.h>       // gettimeofday
#include <unistd.h>         // close

// buffer for data
static char bufo[TBUFLEN+1];

/**
 * @brief my_alloc - safe memory allocation for macro ALLOC
 * @param N - number of elements to allocate
 * @param S - size of single element (typically sizeof)
 * @return pointer to allocated memory area
 */
void *my_alloc(size_t N, size_t S){
    void *p = calloc(N, S);
    if(!p){
        perror("malloc()");
        exit(-1);
    }
    return p;
}

/**
 * @brief dtime - function for different purposes that need to know time intervals
 * @return double value: UNIX time in seconds
 */
double dtime(){
    double t;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec + ((double)tv.tv_usec)/1e6;
    return t;
}

typedef struct {
    int speed;       // communication speed in bauds/s
    tcflag_t bspeed; // baudrate from termios.h
} spdtbl;

static const spdtbl speeds[] = {
    {50, B50},
    {75, B75},
    {110, B110},
    {134, B134},
    {150, B150},
    {200, B200},
    {300, B300},
    {600, B600},
    {1200, B1200},
    {1800, B1800},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000},
    {0,0}
};

/**
 * @brief conv_spd - test if `speed` is in .speed of `speeds` array
 * @param speed - integer speed (bps)
 * @return 0 if error, Bxxx if all OK
 */
static tcflag_t conv_spd(int speed){
    const spdtbl *spd = speeds;
    int curspeed = 0;
    do{
        curspeed = spd->speed;
        if(curspeed == speed)
            return spd->bspeed;
        ++spd;
    }while(curspeed);
    return 0;
}

/**
 * @brief new_tty   - create new TTY structure with partially filled fields
 * @param comdev    - TTY device filename
 * @param speed     - speed (number)
 * @param bufsz     - size of buffer for input data (or 0 if opened only to write)
 * @return pointer to TTY structure if all OK
 */
TTYdescr *new_tty(char *comdev, int speed, size_t bufsz){
    tcflag_t spd = conv_spd(speed);
    if(!spd) return NULL;
    DBG("create %s with speed %d and buffer size %zd", comdev, speed, bufsz);
    TTYdescr *descr = MALLOC(TTYdescr, 1);
    descr->portname = strdup(comdev);
    descr->baudrate = spd;
    descr->speed = speed;
    if(descr->portname && bufsz){
        descr->buf = MALLOC(char, bufsz+1);
        descr->bufsz = bufsz;
        DBG("allocate buffer with size %zd", bufsz);
        return descr;
    }
    FREE(descr->portname);
    FREE(descr);
    return NULL;
}

/**
 * @brief tty_init - open & setup terminal
 * @param descr (io) - port descriptor
 * @return 0 if all OK or error code
 */
int tty_init(TTYdescr *descr){
    DBG("\nOpen port..."); // |O_NONBLOCK
    if ((descr->comfd = open(descr->portname, O_RDWR|O_NOCTTY)) < 0){
        return errno ? errno : 1;
    }
    DBG("OK\nGet current settings...");
    if(tcgetattr(descr->comfd, &descr->oldtty) < 0){ // Get settings
        return errno ? errno : 1;
    }
    descr->tty = descr->oldtty;
    descr->tty.c_lflag     = 0; // ~(ICANON | ECHO | ECHOE | ISIG)
    descr->tty.c_oflag     = 0;
    descr->tty.c_cflag     = descr->baudrate|CS8|CREAD|CLOCAL; // 9.6k, 8N1, RW, ignore line ctrl
    descr->tty.c_cc[VMIN]  = 0;  // non-canonical mode
    descr->tty.c_cc[VTIME] = 5;
    if(tcsetattr(descr->comfd, TCSANOW, &descr->tty) < 0){
        return errno ? errno : 1;
    }
    // make exclusive open
    if(descr->exclusive){
    if(ioctl(descr->comfd, TIOCEXCL)){
        perror("ioctl(TIOCEXCL)");
    }}
    DBG("OK");
    return 0;
}


/**
 * @brief tty_open  - init & open tty device
 * @param d         - already filled structure (with new_tty or by hands)
 * @param exclusive - == 1 to make exclusive open
 * @return pointer to TTY structure if all OK
 */
TTYdescr *tty_open(TTYdescr *d, bool exclusive){
    DBG("open %s with speed %d%s", d->portname, d->speed, exclusive ? "" : " (exclusive)");
    if(!d || !d->portname || !d->baudrate) return NULL;
    if(exclusive) d->exclusive = true;
    else d->exclusive = false;
    if(tty_init(d)) return NULL;
    return d;
}

/**
 * @brief restore_tty - restore opened TTY to previous state and close it
 */
void close_tty(TTYdescr **descr){
    if(descr == NULL || *descr == NULL) return;
    TTYdescr *d = *descr;
    if(d->comfd){
        DBG("close file..");
        ioctl(d->comfd, TCSANOW, &d->oldtty); // return TTY to previous state
        close(d->comfd);
    }
    FREE(d->portname);
    FREE(d->buf);
    FREE(*descr);
    DBG("done!\n");
}

/**
 * @brief read_tty - read data from TTY with 10ms timeout
 * @param buff (o) - buffer for data read
 * @param length   - buffer len
 * @return amount of bytes read
 */
size_t read_tty(TTYdescr *d){
    if(d->comfd < 0) return 0;
    size_t L = 0;
    ssize_t l;
    size_t length = d->bufsz;
    char *ptr = d->buf;
    fd_set rfds;
    struct timeval tv;
    int retval;
    do{
        l = 0;
        FD_ZERO(&rfds);
        FD_SET(d->comfd, &rfds);
        tv.tv_sec = 0; tv.tv_usec = 50000;
        retval = select(d->comfd + 1, &rfds, NULL, NULL, &tv);
        if (!retval) break;
        if(FD_ISSET(d->comfd, &rfds)){
            if((l = read(d->comfd, ptr, length)) < 1){
                break;
            }
            ptr += l; L += l;
            length -= l;
        }
    }while(l && length);
    d->buflen = L;
    d->buf[L] = 0;
    return (size_t)L;
}

/**
 * copy given string to `buf` & add '\n' if need
 * @return 0 if failed
 */
char *cpy2buf(char *string, size_t *l){
    size_t L = strlen(string);
    if(L > TBUFLEN-1){
        return NULL;
    }
    strcpy(bufo, string);
    if(bufo[L-1] != '\n'){
        bufo[L++] = '\n';
        bufo[L] = 0;
    }
    if(l) *l = L;
    return bufo;
}

/**
 * @brief write_tty - write data to serial port
 * @param buff (i)  - data to write
 * @param length    - its length
 * @return 0 if all OK
 */
int write_tty(int comfd, const char *buff, size_t length){
    ssize_t L = write(comfd, buff, length);
    if((size_t)L != length){
        return 1;
    }
    return 0;
}

/**
 * return static buffer - value of `key`
 * NOT THREAD SAFE!
 */
char *keyval(char *key, char *haystack){
    static char buff[32];
    char *got = strstr(haystack, key);
    if(!got) return NULL;
    got = strchr(got, '=');
    if(!got) return NULL;
    ++got;
    char *el = strchr(got, '\n');
    if(!el) return NULL;
    size_t L = (size_t)(el - got);
    if(L > 31 || L == 0 || !*got) return NULL;
    strncpy(buff, got, L);
    buff[L] = 0;
    return buff;
}

/**
 * @brief str2double - safely convert data from string to double
 * @param num (o) - double number read from string
 * @param str (i) - input string
 * @return 1 if success, 0 if fails
 */
int str2double(double *num, const char *str){
    double res;
    char *endptr;
    if(!str) return 0;
    res = strtod(str, &endptr);
    if(endptr == str || *str == '\0' || *endptr != '\0'){
        return false;
    }
    if(num) *num = res; // you may run it like myatod(NULL, str) to test wether str is double number
    return true;
}
