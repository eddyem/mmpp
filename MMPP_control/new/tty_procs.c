/*
 *                                                                                                  geany_encoding=koi8-r
 * tty_procs.c
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "cmdlnopts.h" // for `verbose`
#include "tty_procs.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <usefull_macros.h>

// tty Rx static buffer
#define TBUFLEN         (1024)
// read timeout (in seconds)
#define TTYTIMEOUT      (0.05)

static TTY_descr *dev = NULL;

// two buffers for data
static char buf[TBUFLEN+1];
static char bufo[TBUFLEN+1];

static int motpos[3][2]; // motor's positions, 1 - number of mcu, 2 - number of motor
static int reset[3]; // reset occured
static int endswitches[3][2]; // ESW of motors: 0 - zero esw, 1 - limit esw, -1 - clear
static int alive[3] = {0,0,0}; // ==1 if controller answers, 0 if no

/**
 * @brief tty_tryopen - try to open serial device
 * @param devnm - path to device
 * @param spd   - speed (number)
 * @return 0 if all OK
 */
int tty_tryopen(char *devnm, int spd){
    dev = new_tty(devnm, spd, 256);
    if(!tty_open(dev, TRUE)) return 1;
    return 0;
}

void tty_close(){
    close_tty(&dev);
}

/**
 * read data from TTY
 * WARNING! Not thread-safe!!!
 * @return static buffer with data read or NULL
 */
static char *tty_get(){
    char *ptr = buf;
    size_t L = 0, l = TBUFLEN;
    double t0 = dtime();
    *ptr = 0;
    while(dtime() - t0 < TTYTIMEOUT && l){
        size_t r = read_tty(dev);
        if(!r) continue;
        t0 = dtime();
        if(r > l) r = l;
        DBG("got %zd bytes: %s", r, dev->buf);
        strncpy(ptr, dev->buf, r);
        L += r; l -= r; ptr += r;
    }
    buf[L] = 0;
    if(L){
        return buf;
    }
    DBG("no answer");
    return NULL;
}

/**
 * copy given string to `buf` & add '\n' if need
 * @return 0 if failed
 */
static size_t cpy2buf(char *string){
    size_t l = strlen(string);
    if(l > TBUFLEN-1){
        WARNX(_("String too long! Nothing would be send."));
        return 0;
    }
    strcpy(bufo, string);
    if(bufo[l-1] != '\n'){
        bufo[l++] = '\n';
        bufo[l] = 0;
    }
    return l;
}

/**
 * Send given string command to port
 * @return 0 if failed
 */
static int ttysend(char *cmd){
    size_t l = cpy2buf(cmd);
    if(!l) return 0;
    if(write_tty(dev->comfd, bufo, l)) return 0;
    return 1;
}

/**
 * send RAW string to port device
 * @param string - string to send
 * @return string received or NULL in case of error
 */
char* tty_sendraw(char *string){
    DBG("sendraw %s", string);
    if(!ttysend(string)) return NULL;
    return tty_get();
}

/**
 * Send given string command to port with answer analysis
 * @return status
 */
ttysend_status tty_sendcmd(char *cmd){
    DBG("SEND: %s", cmd);
    if(!ttysend(cmd)) return SEND_ERR;
    char *got = tty_get();
    if(!got) return SEND_ERR;
    DBG("GOT: %s", got);
    if(strcmp(got, "ALLOK\n") == 0) return SEND_ALLOK;
    if(strcmp(got, "OnEndSwitch\n") == 0) return SEND_ESWITCH;
    return SEND_OTHER;
}

/**
 * return static buffer - value of `key`
 * NOT THREAD SAFE!
 */
static char *keyval(char *key){
    //DBG("search %s in\n%s", key, buf);
    static char buff[32];
    char *got = strstr(buf, key);
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
 * parse status with given command `cmd`
 * @return:
 *      1  if one of motors still moving
 *      0  if both are stopped
 *      -1 if failed
 */
static int parsestatus(char *cmd){
    if(!cmd) return 0;
    if(!tty_sendraw(cmd)) return -1;
    char buff[32], stat[2][5], left[2][7], pos[2][7];
    int mvng = 0;
    for(int i = 0; i < 2; ++i){
        sprintf(buff, "POS%d", i);
        char *val = keyval(buff);
        if(!val) return -1;
        strncpy(pos[i], val, 7);
        sprintf(buff, "MOTOR%d", i);
        val = keyval(buff);
        if(!val) return -1;
        if(strcmp(val, "SLEEP") == 0){
            strcpy(stat[i], "STOP");
            strcpy(left[i], "0");
            continue;
        }else if(strcmp(val, "UNKNOWN")){
            strcpy(stat[i], "MOVE");
            mvng = 1;
            sprintf(buff, "STEPSLEFT%d", i);
            val = keyval(buff);
            if(!val) return -1;
            strncpy(left[i], val, 7);
        }else return -1;
    }
    snprintf(bufo, TBUFLEN, "%4s %6s %6s - %4s %6s %6s",
            stat[0], left[0], pos[0], stat[1], left[1], pos[1]);
    if(mvng){
        return 1;
    } else return 0;
}

/**
 * Wait for all motors stop with current data indication
 * @return 0 if all OK
 */
int tty_wait(){
    FNAME();
    int failcount = 0, chk1 = 1, chk2 = 1;
    if(!quiet)
        green("Pol: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS  || L/4: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS \n");
    while(failcount < 5 && (chk1 || chk2)){ // 5 tries
        if(alive[1]){
            chk1 = parsestatus("1GS");
            if(!quiet){
                printf("Pol: ");
                if(chk1 == -1){
                    chk1 = 1;
                    ++failcount;
                    printf("%39s", "failed");
                }else printf("%s", bufo);
            }
        }else if(!quiet) printf("%39s", "failed");
        if(alive[2]){
            chk2 = parsestatus("2GS");
            if(!quiet){
                printf(" || L/4: ");
                if(chk2 == -1){
                    chk2 = 1;
                    ++failcount;
                    printf("%39s", "failed");
                }else printf("%s", bufo);
                printf("   \r");
            }
        }else if(!quiet) printf("%39s", "failed");
    }
    if(!quiet) printf("\n\n");
    if(failcount > 4){
        WARNX(_("Can't get status answer!"));
        return 1;
    }
    return 0;
}

#if 0
MOTOR0=SLEEP
POS0=43
ESW00=RLSD
ESW01=HALL
MOTOR1=MOVE
STEPSLEFT1=6921
POS1=3238
ESW10=HALL
ESW11=RLSD
#endif

// tty_getstatus when `quiet==1`
static void ttystatq(){
    char st[4] = "xGS";
    char *nm[3] = {NULL, "POL", "L4"};
    for(int Nmcu = 1; Nmcu < 3; ++Nmcu){
        if(!alive[Nmcu]) continue;
        st[0] = '0' + Nmcu;
        if(!tty_sendraw(st)) continue;
        char *str = strtok(buf, "\n");
        while(str){
            printf("%s%s\n", nm[Nmcu], str);
            str = strtok(NULL, "\n");
        }
    }
}

// tty_getstatus when `quiet==0`
static void ttystat(){
    int chk1 = -1, chk2 = -1;
    char buff[TBUFLEN+1];
    if(!quiet) green("Pol: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS  || L/4: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS \n");
    if(alive[1]) chk1 = parsestatus("1GS");
    if(!quiet) printf("Pol: ");
    if(chk1 == -1){
        if(!quiet) printf("%39s", "failed");
    }else printf("%s", bufo);
    char *val = keyval("ESW00");
    if(!val) val = " ";
    sprintf(buff, "%5s ", val);
    val = keyval("ESW01");
    if(!val) val = " ";
    sprintf(&buff[6], "%5s ", val);
    val = keyval("ESW10");
    if(!val) val = " ";
    sprintf(&buff[12], "%5s ", val);
    val = keyval("ESW11");
    if(!val) val = " ";
    sprintf(&buff[18], "%5s ", val);
    if(alive[2]) chk2 = parsestatus("2GS");
    printf(" || L/4: ");
    if(chk2 == -1){
        printf("%39s", "failed");
    }else printf("%s", bufo);
    printf("\n");
    sprintf(&buff[24], "|| ");
    val = keyval("ESW00");
    DBG("ESW00=%s", val);
    if(!val) val = " ";
    sprintf(&buff[27], "%5s ", val);
    val = keyval("ESW01");
    if(!val) val = " ";
    sprintf(&buff[33], "%5s ", val);
    val = keyval("ESW10");
    if(!val) val = " ";
    sprintf(&buff[39], "%5s ", val);
    val = keyval("ESW11");
    if(!val) val = " ";
    sprintf(&buff[45], "%5s ", val);
    // end-switches
    green("ESW00 ESW01 ESW10 ESW11 || ESW00 ESW01 ESW10 ESW11\n");
    printf("%s\n", buff);
    for(int i = 1; i < 3; ++i){
        if(reset[i]) printf("RESET%d=1\n", i);
    }
}

void tty_getstatus(){
    FNAME();
    if(quiet) ttystatq();
    else ttystat();
}


/**
 * Show temperature of both MCU
 * @return amount of successful calls
 */
int tty_showtemp(){
    char *val, buff[] = "xGT\n";
    const char *nm[3] = {NULL, "POL", "L4"};
    int ret = 0;
    for(int i = 1; i < 3; ++i){
        if(!alive[i]){
            DBG("MCU %d didn't respond!", i);
            continue;
        }
        buff[0] = '0' + (char)i;
        if(tty_sendraw(buff)){
            val = keyval("TEMP");
            if(val){
                ++ret;
                double t;
                if(str2double(&t, val)){
                    if(quiet){
                        printf("%sTEMP=%g\n", nm[i], t/10.);
                    }else{
                        green("MCU%d temperature:", i);
                        printf(" %g degC\n", t/10.);
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * get current motor position (if move or error return INT_MIN)
 *
 */
int mot_getpos(int mcu, int motor){
    if(mcu < 1 || mcu > 2){
        WARNX(_("Bad MCU number"));
        return INT_MIN;
    }
    if(motor < 0 || motor > 1){
        WARNX(_("Bad motor number"));
        return INT_MIN;
    }
    return motpos[mcu][motor];
}

/**
 * get motor endswitches status
 * @return -1 if error or intermediate position, 0 - on zero's esw, 1 - on end's esw
 */
int mot_getesw(int mcu, int motor){
    if(mcu < 1 || mcu > 2){
        WARNX(_("Bad MCU number"));
        return -1;
    }
    if(motor < 0 || motor > 1){
        WARNX(_("Bad motor number"));
        return -1;
    }
    return endswitches[mcu][motor];
}

/**
 * test connection (1,2 -> ALIVE)
 * and get positions
 * @return 1 if none of MCU found, 0 if at least 1 found
 */
int handshake(){
    char buff[32], *val;
    int mcu, motor;
    FNAME();
    for(mcu = 1; mcu < 3; ++ mcu){
        // check if MCU alive
        sprintf(buff, "%d", mcu);
        int notresp = 1;
        // make HANDSHAKE_TRIES tries
        for(int tr = 0; tr < HANDSHAKE_TRIES; ++tr){
            if(tty_sendraw(buff) && 0 == strcmp(buf, "ALIVE\n")){
                notresp = 0;
                break;
            }
        }
        if(notresp){
            WARNX(_("MCU %d not response!"), mcu);
            continue;
        }
        alive[mcu] = 1;
        sprintf(buff, "%dGS", mcu);
        if(tty_sendraw(buff)){
            // check reboot states
            if(strstr(buf, "RESET")){
                reset[mcu] = 1;
                WARNX(_("MCU %d had reset state!"), mcu);
            }else
                reset[mcu] = 0;
            for(motor = 0; motor < 2; ++motor){
                /*sprintf(buff, "MOTOR%d", motor);
                val = keyval(buff);
                if(!val || strcmp(val, "SLEEP")){
                    motpos[mcu][motor] = INT_MIN;
                    continue;
                }*/
                sprintf(buff, "POS%d", motor);
                val = keyval(buff);
                DBG("----%s=%s", buff, val);
                motpos[mcu][motor] = val ? atoi(val) : INT_MIN;
                DBG("MOTPOS[%d][%d] = %d", mcu, motor, motpos[mcu][motor]);
                // end-switches
                sprintf(buff, "ESW%d0", motor);
                val = keyval(buff);
                if(!val || strcmp(val, "HALL")){
                    sprintf(buff, "ESW%d1", motor);
                    val = keyval(buff);
                    if(!val || strcmp(val, "HALL")){
                        endswitches[mcu][motor] = -1;
                    }else
                        endswitches[mcu][motor] = 1;
                }else endswitches[mcu][motor] = 0;
                DBG("ENDSWITCHES[%d][%d] = %d", mcu, motor, endswitches[mcu][motor]);
            }
        }else{
            motpos[mcu][0] = INT_MIN;
            motpos[mcu][1] = INT_MIN;
        }
    }
    if(alive[1] == 0 && alive[2] == 0) return 1;
    return 0;
}

/**
 * @brief tty_stopall - send commands to stop all motors
 * @return 0 if all OK, else return amount of motors failed to stop
 */
int tty_stopall(){
    int ret = 4;
    if(alive[1]){
        if(SEND_ALLOK == tty_sendcmd("1M0S")) --ret;
        if(SEND_ALLOK == tty_sendcmd("1M1S")) --ret;
    }
    if(alive[2]){
        if(SEND_ALLOK == tty_sendcmd("2M0S")) --ret;
        if(SEND_ALLOK == tty_sendcmd("2M1S")) --ret;
    }
    return ret;
}

/**
 * @brief init_motors - init all motors simultaneously (if they need to)
 * @return 0 if all OK, or Nmcu*10+motnum for problem motor
 */
int init_motors(){
#define RETVAL() (Nmcu*10+motnum)
    int Nmcu, motnum, needinit = 0;
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
    for(motnum = 0; motnum < 2; ++motnum){
        // check position
        int pos = motpos[Nmcu][motnum];
        if(pos == INT_MIN) continue; // communication error on handshake
        if(pos < 0) needinit = 1;
    }}
    if(!needinit) return 0;
    DBG("Need to init, start!");
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
    for(motnum = 0; motnum < 2; ++motnum){
        int pos = motpos[Nmcu][motnum];
        if(pos >= 0) continue;
        // check if we are on zero endswitch
        int esw = mot_getesw(Nmcu, motnum);
        if(esw == 0){ // move a little from zero esw
            sprintf(buf, "%dM%dM100", Nmcu, motnum);
            if(SEND_ERR == tty_sendcmd(buf)){
                WARNX(_("Can't move from endswitch"));
                return RETVAL();
            }
            tty_wait();
        }
        sprintf(buf, "%dM%dM-40000", Nmcu, motnum);
        if(SEND_ALLOK != tty_sendcmd(buf)){
            WARNX(_("Can't move to endwsitch 0"));
            return RETVAL();
        }
    }}
    tty_wait();
    handshake();
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
    for(motnum = 0; motnum < 2; ++motnum){
        int curpos = mot_getpos(Nmcu, motnum);
        if(curpos){
            WARNX(_("Can't return to zero"));
            return RETVAL();
        }
    }}
    return 0;
#undef RETVAL
}

