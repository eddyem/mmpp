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

#include "tty_procs.h"
#include "cmdlnopts.h" // for `verbose`
#include "usefull_macros.h"

// tty Rx static buffer
#define TBUFLEN         (1024)
// read timeout (in seconds)
#define TTYTIMEOUT      (0.05)

// two buffers for data
static char buf[TBUFLEN+1];
static char bufo[TBUFLEN+1];

static int motpos[3][2]; // motor's positions, 1 - number of mcu, 2 - number of motor
static int reset[3]; // reset occured
static int endswitches[3][2]; // ESW of motors: 0 - zero esw, 1 - limit esw, -1 - clear

/**
 * read data from TTY
 * WARNING! Not thread-safe!!!
 * @return static buffer with data read or NULL
 */
static char *tty_get(){
    char *ptr = buf;
    size_t L = 0, l = TBUFLEN;
//    double t0 = dtime();
//    while(dtime() - t0 < TTYTIMEOUT){
    while(1){
        size_t r = read_tty(ptr, l);
        if(!r) break;
        /*if(r) t0 = dtime();
        else{
            DBG("Nothing");
            continue;
        }*/
        ptr[r] = 0;
        DBG("next portion: %s", ptr);
        L += r; l -= r; ptr += r;
    }
    if(L) return buf;
    return NULL;
}

/**
 * copy given string to `buf` & add '\n' if need
 * @return 0 if failed
 */
static size_t cpy2buf(char *string){
    size_t l = strlen(string);
    if(l > TBUFLEN){
        WARNX(_("String too long! Nothing would be send."));
        return 0;
    }
    strcpy(buf, string);
    if(buf[l-1] != '\n'){
        DBG("add \\n");
        buf[l++] = '\n';
        buf[l] = 0;
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
    if(write_tty(buf, l)) return 0;
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
 * Send given motor string command to port with answer analysis
 * @return:
 *      -1  if failed
 *      0   if started moving
 *      1   if on endswitch and can't move further
 *      2   if other error occured
 */
int tty_sendcmd(char *cmd){
    if(!ttysend(cmd)) return -1;
    char *got = tty_get();
    if(!got) return -1;
    if(strcmp(got, "ALL OK\n") == 0) return 0;
    if(strcmp(got, "OnEndSwitch\n") == 0) return 1;
    return 2;
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
    size_t L = el - got;
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
 */
void tty_wait(){
    FNAME();
    int failcount = 0, chk1 = 1, chk2 = 1;
    if(!quiet)
        green("Pol: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS  || L/4: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS \n");
    while(failcount < 5 && (chk1 || chk2)){ // 5 tries
        chk1 = parsestatus("1GS");
        if(!quiet){
            printf("Pol: ");
            if(chk1 == -1){
                chk1 = 1;
                ++failcount;
                printf("%39s", "failed");
            }else printf("%s", bufo);
        }
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
        //usleep(100000);
    }
    if(!quiet) printf("\n\n");
    if(failcount > 4) WARNX(_("Can't get status answer!"));
}

void tty_getstatus(){
    FNAME();
    int chk1, chk2;
    char buff[TBUFLEN+1];
    if(!quiet)
        green("Pol: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS  || L/4: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS \n");
    chk1 = parsestatus("1GS");
    printf("Pol: ");
    if(chk1 == -1){
        printf("%39s", "failed");
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
    chk2 = parsestatus("2GS");
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
    if(!quiet) green("ESW00 ESW01 ESW10 ESW11 || ESW00 ESW01 ESW10 ESW11\n");
    printf("%s\n", buff);
    for(int i = 1; i < 3; ++i){
        if(reset[i]) printf("RESET%d=1\n", i);
    }
}


/**
 * Show temperature of both MCU
 */
void tty_showtemp(){
    char *val, buff[] = "xGT\n";
    for(int i = 1; i < 3; ++i){
        buff[0] = '0' + i;
        if(tty_sendraw(buff)){
            val = keyval("TEMP");
            if(val){
                double t;
                if(str2double(&t, val)){
                    green("MCU%d temperature: %g degC\n", i, t/10.);
                }
            }
        }
    }
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
 *
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
 */
void handshake(){
    char buff[32], *val;
    int mcu, motor;
    for(mcu = 1; mcu < 3; ++ mcu){
        // check if MCU alive
        sprintf(buff, "%d", mcu);
        int notresp = 0;
        if(!tty_sendraw(buff)) notresp = 1;
        else if(strcmp(buf, "ALIVE\n")) notresp = 1;
        if(notresp){
            WARNX(_("MCU %d not response!"), mcu);
            continue;
        }
        sprintf(buff, "%dGS", mcu);
        if(tty_sendraw(buff)){
            // check reboot states
            if(strstr(buf, "RESET")){
                reset[mcu] = 1;
                WARNX(_("MCU %d had reset state!"), mcu);
            }else
                reset[mcu] = 0;
            for(motor = 0; motor < 2; ++motor){
                sprintf(buff, "MOTOR%d", motor);
                val = keyval(buff);
                if(!val || strcmp(val, "SLEEP")){
                    motpos[mcu][motor] = INT_MIN;
                    continue;
                }
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
}
