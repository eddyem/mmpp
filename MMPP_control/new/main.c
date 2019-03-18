/*
 *                                                                                                  geany_encoding=koi8-r
 * main.c
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

#include "cmdlnopts.h"
#include "signal.h"
#include "tty_procs.h"
#include <stdio.h>
#include <string.h>
#include <usefull_macros.h>

// All return states of main():
enum{
    RET_ALLOK = 0,
    RET_NOTFOUND,       // none of MCU found or didn't found seeking MCU
    RET_ONLYONE,        // only one MCU found
    RET_COMMERR,        // communication error
    RET_CANTINIT,       // can't init motors: error during initiation or some of motors are moving
    RET_WAITERR,        // error occured during waiting procedure
    RET_ERROR = 9,      // uncoverable error - from libsnippets
    RET_HELPCALL = 255  // user call help (or give wrong parameter[s]) - from libsnippets
};

static glob_pars *G;

/**
 * We REDEFINE the default WEAK function of signal processing
 */
void __attribute__((noreturn)) signals(int sig){
    if(sig){
        signal(sig, SIG_IGN);
        DBG("Get signal %d, quit.\n", sig);
    }
    if(G->pidfile) // remove unnesessary PID file
        unlink(G->pidfile);
    restore_console();
    tty_close();
    exit(sig);
}

void __attribute__((noreturn)) iffound_default(pid_t pid){
    ERRX("Another copy of this process found, pid=%d. Exit.", pid);
}

static double convangle(double val){
    int X = (int)(val / 360.);
    val -= 360. * (double)X;
    return val;
}

/**
 * @return 1 if motor start moving else return 0
 */
static int parsemotans(ttysend_status ans, const char *prefix){
    DBG("parse ans: %d (prefix: %s)", ans, prefix);
    if(ans == SEND_ALLOK) return 1;
    else if(ans == SEND_ERR) WARNX(_("%s moving error!"), prefix);
    else if(ans == SEND_ESWITCH) WARNX(_("%s is on end-switch and can't move further"), prefix);
    else if(ans == SEND_OTHER) WARNX(_("Can't move %s: bad steps amount or still moving"), prefix);
    return 0;
}

/**
 * move motor
 * @return 0 if motor can't move, else return 1
 */
/**
 * @brief movemotor - move motor
 * @param mcu     - MCU# (controller No, 1 or 2)
 * @param motnum  - motor# (0 or 1)
 * @param steps   - steps amount (
 * @param name
 * @return 0 if motor can't move, else return 1
 */
static int movemotor(int mcu, int motnum, int steps, const char *name){
    char buf[32];
    int curpos = mot_getpos(mcu, motnum);
    if(curpos == INT_MIN){
        WARNX(_("Can't get current %s position"), name);
        return 0;
    }
    if(curpos < 0){ // need to init
        WARNX(_("Init of %s failed"), name);
        return 0;
    }
    if(G->absmove){
        if(steps < 0){
            if(motnum == 1){
                steps += (mcu == 1) ? STEPSREV1 : STEPSREV2; // convert rotator angle to positive
            }else{
                WARNX(_("Can't move to negative position"));
                return 0;
            }
        }
        steps -= curpos;
    }
    if(steps == 0){
        MSG(name, _("already at position"));
        return 0;
    }
    DBG("try to move motor%d of mcu %d for %d steps", motnum, mcu, steps);
    snprintf(buf, 32, "%dM%dM%d", mcu, motnum, steps);
    ttysend_status ans = tty_sendcmd(buf);
    return parsemotans(ans, name);
}

int main(int argc, char **argv){
//    char cmd[32];
    int waitforstop = 0;
    int rtn_status = RET_ALLOK;
    initial_setup();
    signal(SIGTERM, signals);   // kill (-15)
    signal(SIGINT, signals);    // ctrl+C
    signal(SIGQUIT, SIG_IGN);   // ctrl+\   .
    signal(SIGTSTP, SIG_IGN);   // ctrl+Z
    setbuf(stdout, NULL);
    G = parse_args(argc, argv);
    char *self = strdup(argv[0]);
    check4running(self, G->pidfile);
    DBG("Try to open serial %s", G->comdev);
    if(tty_tryopen(G->comdev, G->speed)){
        ERR(_("Can't open %s with speed %d. Exit."), G->comdev, G->speed);
    }

    if(handshake()) signals(RET_NOTFOUND); // test connection & get all positions
    if(G->waitold){
        if(tty_wait()) signals(RET_WAITERR);
        handshake();
    }
    if(G->showtemp){
        if(tty_showtemp() != 2) rtn_status = RET_ONLYONE;
    }
    if(G->stopall){ // stop everything before analyze other commands
        if(tty_stopall()) rtn_status = RET_COMMERR;
        else MSG(_("All motors stopped"), NULL);
    }
    if(G->sendraw){
        MSG(_("Send raw string"), G->sendraw);
        char *got = tty_sendraw(G->sendraw);
        if(got){
            MSG(_("Receive"), got);
            if(quiet) printf("%s", got);
        }else WARNX(_("Nothing received"));
    }
    if(G->rot1angle > -999. || G->rot2angle > -999. || G->l1steps != INT_MAX || G->l2steps != INT_MAX){
        // all other commands are tied with moving, so check if motors are inited
        if(init_motors()) rtn_status = RET_CANTINIT;
        else{
            if(G->rot1angle > -999.){
                double angle = convangle(G->rot1angle);
                int steps = (int)((STEPSREV1/360.) * angle);
                waitforstop = movemotor(1, 1, steps, "polaroid");
            }
            if(G->rot2angle > -999.){
                double angle = convangle(G->rot2angle);
                int steps = (int)((STEPSREV2/360.) * angle);
                waitforstop = movemotor(2, 1, steps, "lambda/4");
            }
            if(G->l1steps != INT_MAX){
                waitforstop = movemotor(1, 0, G->l1steps, "polaroid stage");
            }
            if(G->l2steps != INT_MAX){
                waitforstop = movemotor(2, 0, G->l2steps, "lambda/4 stage");
            }
        }
    }
    if((waitforstop && !G->dontwait)) if(tty_wait()) rtn_status = RET_WAITERR;
    if(G->getstatus) tty_getstatus();
    if(G->reset){
        int **N = G->reset;
        while(*N){
            char cmd[3];
            if(**N < 1 || **N > 2){
                WARNX(_("Wrong MCU number (%d)"), **N);
            }else{
                if(!quiet) green("Reset controller #%d\n", **N);
                snprintf(cmd, 3, "%dR", **N);
                ttysend_status _U_ rt = tty_sendcmd(cmd);
                DBG("reset %d, result: %d", **N, rt);
            }
            ++N;
        }
    }
    signals(rtn_status);
}
