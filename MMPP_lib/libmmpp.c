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

#include "libmmpp.h"
#include "tty_procs.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined GETTEXT
#include <libintl.h>
#define _(String)               gettext(String)
#define gettext_noop(String)    String
#define N_(String)              gettext_noop(String)
#else
#define _(String)               (String)
#define N_(String)              (String)
#endif

// tty device
static TTYdescr *dev = NULL;
static bool alive[3] = {false,true,true}; // ==true if controller answers, false if no
static bool reset[3] = {false,false,false}; // reset occured

/**
 * @brief tty_tryopen - try to open serial device
 * @param devnm - path to device
 * @param spd   - speed (number)
 * @return 0 if all OK
 */
int mmpp_tryopen(char *devnm, int spd){
    if(dev) close_tty(&dev);
    dev = new_tty(devnm, spd, 256);
    if(!dev) return 1;
    if(!tty_open(dev, true)) return 1;
    return 0;
}

/**
 * @brief tty_close - close TTY device
 */
void mmpp_close(){
    if(!dev) return;
    close_tty(&dev);
}

/**
 * test connection (1,2 -> ALIVE)
 * @return 1 if none of MCU found, 0 if at least 1 found
 */
int mot_handshake(){
    char buff[32], *ret;
    int mcu;
    FNAME();
    for(mcu = 1; mcu < 3; ++mcu){
        DBG("MCU #%d", mcu);
        // check if MCU alive
        sprintf(buff, "%d", mcu);
        int notresp = 1;
        alive[mcu] = false;
        // make HANDSHAKE_TRIES tries
        for(int tr = 0; tr < HANDSHAKE_TRIES; ++tr){
            ret = tty_sendraw(buff);
            if(ret && 0 == strcmp(ret, "ALIVE\n")){
                notresp = 0;
                break;
            }
        }
        if(notresp){
            continue;
        }
        alive[mcu] = true;
    }
    if(alive[1] == false && alive[2] == false) return 1;
    return 0;
}

/**
 * Get temperature of both MCUs
 * @param t1, t2 (o) - temperatures of first and second MCUs (==-300 if error)
 * @return amount of successful calls
 */
int get_temp(double *t1, double *t2){
    char *val, buff[] = "xGT\n", *ans;
    int ret = 0;
    if(t1) *t1 = -300.;
    if(t2) *t2 = -300.;
    for(int i = 1; i < 3; ++i){
        if(!alive[i]){
            DBG("MCU %d didn't respond!", i);
            continue;
        }
        buff[0] = '0' + (char)i;
        if((ans = tty_sendraw(buff))){
            val = keyval("TEMP", ans);
            DBG("val: %s", val);
            if(val){
                ++ret;
                double t;
                if(str2double(&t, val)){
                    if(i == 1){if(t1) *t1 = t/10.;}
                    else{if(t2) *t2 = t/10.;}
                }
            }
        }
    }
    return ret;
}

/**
 * @brief stop_all - send commands to stop all motors
 * @return 0 if all OK, else return amount of motors failed to stop
 */
int stop_all(){
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
 * @brief init_motors - BLOCKING (!!!) init all motors simultaneously (if they need to)
 * @return 0 if all OK, or Nmcu*10+motnum for problem motor (motnum == 2 for problems with MCU)
 * !!! in case of non-zero returning you should repeat initialisation
 */
int init_motors(){
    FNAME();
    char buf[32];
    motor_state S[3];
#define RETVAL() (Nmcu*10+motnum)
    int Nmcu, motnum, needinit = 0;
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
        if(!mot_getstatus(Nmcu, &S[Nmcu])) return 10*Nmcu+2;
        for(motnum = 0; motnum < 2; ++motnum){
            // check position
            if(S[Nmcu].curpos[motnum] < 0) needinit = 1;
        }
    }
    if(!needinit) return 0;
    red("Need to init, start!\n");
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
    for(motnum = 0; motnum < 2; ++motnum){
        int pos = S[Nmcu].curpos[motnum];
        if(pos >= 0) continue;
        // check if we are on zero endswitch
        if(S[Nmcu].ESW_status[motnum][0] == ESW_HALL){ // move a little from zero esw
            red("from esw\n");
            sprintf(buf, "%dM%dM100", Nmcu, motnum);
            if(SEND_ERR == tty_sendcmd(buf)){
                return RETVAL();
            }
            mot_wait();
        }
        sprintf(buf, "%dM%dM-40000", Nmcu, motnum);
        if(SEND_ALLOK != tty_sendcmd(buf)){
            return RETVAL();
        }
    }}
    mot_wait();
    green("check\n");
    // check current positions
    for(Nmcu = 1; Nmcu < 3; ++Nmcu){
        if(!mot_getstatus(Nmcu, &S[Nmcu])) return 10*Nmcu+2;
        for(motnum = 0; motnum < 2; ++motnum){
            if(S[Nmcu].curpos[motnum]){
                return RETVAL();
            }
        }
    }
    return 0;
#undef RETVAL
}

/**
 * Wait while all motors are stopped
 * @return 0 if all OK
 */
int mot_wait(){
    int failcount = 0;
    bool mov[3] = {false,true, true};
    if(!alive[1] && !alive[2]) return 0; // all are dead here
    while(failcount < FAIL_TRIES && (mov[1] || mov[2])){
        for(int Nmcu = 1; Nmcu < 3; ++Nmcu){
            usleep(10000);
            DBG("alive=%d/%d, mov=%d/%d", alive[1],alive[2],mov[1],mov[2]);
            if(!alive[Nmcu] || !mov[Nmcu]) continue;
            motor_state S;
            if(!mot_getstatus(Nmcu, &S)){
                ++failcount;
            }else failcount = 0;
            if(S.state[0] == STP_SLEEP && S.state[1] == STP_SLEEP)
                mov[Nmcu] = false;
        }
    }
    if(failcount >= FAIL_TRIES){
        return 1;
    }
    return 0;
}

/**
 * read data from TTY
 * WARNING! Not thread-safe!!!
 * @return static buffer with data read or NULL
 */
char *tty_get(){
    if(!dev) return NULL;
    static char buf[TBUFLEN];
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
 * Send given string command to port
 * @return 0 if failed
 */
int tty_send(char *cmd){
    if(!dev) return 0;
    size_t l = 0;
    char *s = cpy2buf(cmd, &l);
    if(!s) return 0;
    if(write_tty(dev->comfd, s, l)) return 0;
    return 1;
}

/**
 * send RAW string to port device
 * @param string - string to send
 * @return string received or NULL in case of error
 */
char* tty_sendraw(char *string){
    if(!dev) return NULL;
    DBG("sendraw %s", string);
    if(!tty_send(string)) return NULL;
    return tty_get();
}

/**
 * Send given string command to port with answer analysis
 * @return status
 */
ttysend_status tty_sendcmd(char *cmd){
    DBG("SEND: %s", cmd);
    if(!tty_send(cmd)) return SEND_ERR;
    char *got = tty_get();
    if(!got) return SEND_ERR;
    if(strcmp(got, "ALLOK\n") == 0) return SEND_ALLOK;
    else if(strcmp(got, "IsMoving") == 0) return SEND_ACTIVE;
    else if(strcmp(got, "OnEndSwitch\n") == 0) return SEND_ESWITCH;
    else if(strcmp(got, "ZeroMove") == 0) return SEND_ZEROMOVE;
    else if(strcmp(got, "TooBigNumber") == 0) return SEND_TOOBIG;
    return SEND_OTHER;
}

/**
 * @brief mot_getstatus - get status of motors for given controller
 * @param Nmcu    - (1 or 2) MCU number
 * @param s   (o) - status
 * @return true if all OK
 */
bool mot_getstatus(int Nmcu, motor_state *s){
    char buff[32], *ans, cmd[4] = "xGS", *val;
    if(Nmcu < 1 || Nmcu > 2) return false;
    cmd[0] = '0' + Nmcu;
    ans = tty_sendraw(cmd);
    if(!ans){
        alive[Nmcu] = false;
        return false;
    }
    alive[Nmcu] = true;
    motor_state S;
    memset(&S, 0, sizeof(S));
    val = keyval("WDGRESET", ans);
    if(val) s->rst = RESET_WDG;
    else{
        val = keyval("SOFTRESET", ans);
        if(val) s->rst = RESET_SW;
    }
    if(s->rst != RESET_NONE) reset[Nmcu] = true;
    for(int i = 0; i < 2; ++i){
        sprintf(buff, "POS%d", i);
        val = keyval(buff, ans);
        if(val){
            S.curpos[i] = atoi(val);
        }
        sprintf(buff, "MOTOR%d", i);
        val = keyval(buff, ans);
        stp_state ms = STP_UNKNOWN;
        if(val){
            if(strcmp(val, "ACCEL") == 0) ms = STP_ACCEL;
            else if(strcmp(val, "DECEL") == 0) ms = STP_DECEL;
            else if(strcmp(val, "MOVE") == 0) ms = STP_MOVE;
            else if(strcmp(val, "MOVETO0") == 0) ms = STP_MOVE0;
            else if(strcmp(val, "MOVETO1") == 0) ms = STP_MOVE1;
            else if(strcmp(val, "MVSLOW") == 0) ms = STP_MVSLOW;
            else if(strcmp(val, "STOP") == 0) ms = STP_STOP;
            else if(strcmp(val, "STOPZERO") == 0) ms = STP_STOPZERO;
            else if(strcmp(val, "SLEEP") == 0) ms = STP_SLEEP;
            S.state[i] = ms;
        }
        if(ms != STP_UNKNOWN && ms != STP_SLEEP){ // moving
            sprintf(buff, "STEPSLEFT%d", i);
            val = keyval(buff, ans);
            if(val){
                S.stepsleft[i] = atoi(val);
            }
        }
        // end-switches
        for(int j = 0; j < 2; ++j){
            sprintf(buff, "ESW%d%d", i, j);
            val = keyval(buff, ans);
            if(val){
                ESW_status s = ESW_ERROR;
                if(strcmp(val, "RLSD") == 0) s = ESW_RELEASED;
                else if(strcmp(val, "BTN") == 0) s = ESW_BUTTON;
                else if(strcmp(val, "HALL") == 0) s = ESW_HALL;
                S.ESW_status[i][j] = s;
            }
        }
    }
    if(s) *s = S;
    return true;
}

/**
 * @brief get_rst - get reset state for mcu #N
 * @param N       - number of MCU
 * @param clear   - true to clear reset state
 * @return
 */
bool get_rst(int N, bool clear){
    if(N < 1 || N > 2) return false;
    bool state = reset[N];
    if(clear) reset[N] = 0;
    return state;
}

/**
 * @brief get_alive - return true if MCU #N is alive
 * @param N - number of MCU (1 or 2)
 * @return alive[N]
 */
bool get_alive(int N){
    if(N < 1 || N > 2) return false;
    return alive[N];
}

/**
 * @brief get_ADC - ADC values
 * @param N     - number of MCU (1 or 2)
 * @param s (o) - state of all ADC channels
 * @return true if all OK
 */
bool get_ADC(int N, ADC_state *s){
    if(N < 1 || N > 2 || !s || !alive[N]) return false;
    char buff[] = "xGAy", cmds[3] = "DIM", *ansv[] = {"VDD", "IMOT", "VMOT"};
    double *vals[3] = {&s->Vdd, &s->Imot, &s->Vmot};
    int got = 0;
    buff[0] = '0' + N;
    for(int i = 0; i < 3; ++i){
        buff[3] = cmds[i];
        char *ans = tty_sendraw(buff);
        if(!ans) continue;
        char *v = keyval(ansv[i], ans);
        if(v){
            double t;
            if(str2double(&t, v)){
                *vals[i] = t / 100.;
                DBG("Got %s=%g", ansv[i], *vals[i]);
                ++got;
            }
        }
    }
    if(got != 3) return false;
    return true;
}

/**
 * @brief reset_MCU - reset given controller
 * @param N - MCU # (1 or 2)
 */
void reset_MCU(int N){
    if(N < 1 || N > 2) return;
    char cmd[] = "xR";
    cmd[0] = '0' + N;
    tty_sendraw(cmd);
}

/**
 * @brief movemotor - move motor
 * @param mcu     - MCU# (controller No, 1 or 2)
 * @param motnum  - motor# (0 or 1)
 * @param steps   - steps amount
 * @param absmove - !=0 if steps are absolute position
 * @return
 */
ttysend_status movemotor(int mcu, int motnum, int steps, int absmove){
    if(mcu < 1 || mcu > 3 || motnum < 0 || motnum > 1) return SEND_OTHER;
    char buf[32];
    motor_state mstate;
    if(!mot_getstatus(mcu, &mstate)) return SEND_ERR;
    if(mstate.state[motnum] != STP_SLEEP) return SEND_ACTIVE;
    int curpos = mstate.curpos[motnum];
    if(curpos < 0){ // need to init
        return SEND_NEEDINIT;
    }
    if(absmove){
        if(motnum == 1){ // convert rotator angle to positive
            int perrev = (mcu == 1) ? STEPSREV1 : STEPSREV2;
            steps %= perrev;
            if(steps < 0) steps += perrev;
        }
        if(steps < 0){
            return SEND_NEGATMOVE;
        }
        steps -= curpos;
    }
    if(steps == 0){
        return SEND_ZEROMOVE;
    }
    DBG("try to move motor%d of mcu %d for %d steps", motnum, mcu, steps);
    snprintf(buf, 32, "%dM%dM%d", mcu, motnum, steps);
    return tty_sendcmd(buf);
}
