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

#include "usefull_macros.h"
#include "cmdlnopts.h"
#include "tty_procs.h"
#include "signal.h"

static glob_pars *G;

void signals(int signo){
    restore_console();
    restore_tty();
    exit(signo);
}

static void MSG(const char *s1, const char *s2){
    DBG("%s: %s", s1, s2);
    if(quiet) return;
    if(s1) green("%s: ", s1);
    if(s2) printf("%s\n", s2);
}

static double convangle(double val){
    int X = val / 360.;
    val -= 360. * (double)X;
    return val;
}

/**
 * @return 1 if motor start moving else return 0
 */
static int parsemotans(int ans, const char *prefix){
    if(ans == 0) return 1;
    else if(ans == -1) WARNX(_("%s moving error!"), prefix);
    else if(ans == 1) WARNX(_("%s is on end-switch and can't move further"), prefix);
    else if(ans == 2) WARNX(_("Can't move %s: bad steps amount or still moving"), prefix);
    return 0;
}

/**
 * move motor
 * @return waitforstop value
 */
int movemotor(int mcu, int motnum, int steps, const char *name){
    char buf[32];
    int curpos = mot_getpos(mcu, motnum);
    if(curpos == INT_MIN){
        WARNX(_("Can't get current %s position"), name);
        return 0;
    }
    if(G->absmove){
        if(curpos < 0){ // need to init
            // check if we are on zero endswitch
            int esw = mot_getesw(mcu, motnum);
            if(esw == 0){ // move a little
                sprintf(buf, "%dM%dM100", mcu, motnum);
                tty_sendcmd(buf);
                tty_wait();
            }
            sprintf(buf, "%dM%dM-40000", mcu, motnum);
            if(tty_sendcmd(buf)){
                WARNX(_("Can't initialize %s"), name);
                return 0;
            }
            tty_wait(); // wait for initialisation ends
            handshake();
            curpos = mot_getpos(mcu, motnum);
            if(curpos){
                WARNX(_("Can't return to zero"));
                return 0;
            }
        }
        if(steps < 0){
            if(motnum == 1){
                steps += (mcu == 1) ? 36000 : 28800; // convert rotator angle to positive
            }else{
                WARNX(_("Can't move to negative position"));
                return 0;
            }
        }
        steps -= curpos;
    }
    if(steps == 0){
        MSG(name, "already at position");
        return 0;
    }
    DBG("try to move motor%d of mcu %d for %d steps", motnum, mcu, steps);
    snprintf(buf, 32, "%dM%dM%d", mcu, motnum, steps);
    int ans = tty_sendcmd(buf);
    return parsemotans(ans, name);
}

int main(int argc, char **argv){
//    char cmd[32];
    int waitforstop = 0;
    signal(SIGTERM, signals);   // kill (-15)
    signal(SIGINT, signals);    // ctrl+C
    signal(SIGQUIT, SIG_IGN);   // ctrl+\   .
    signal(SIGTSTP, SIG_IGN);   // ctrl+Z
    setbuf(stdout, NULL);
    initial_setup();
    G = parse_args(argc, argv);
    setup_con(); // no echo
    tty_init(G->comdev);
    handshake(); // test connection & get all positions
    if(G->showtemp){
        tty_showtemp();
    }
    if(G->stopall){ // stop everything before analyze other commands
        tty_sendcmd("1M0S");
        tty_sendcmd("1M1S");
        tty_sendcmd("2M0S");
        tty_sendcmd("2M1S");
    }
    if(G->sendraw){
        MSG(_("Send raw string"), G->sendraw);
        char *got = tty_sendraw(G->sendraw);
        if(got){
            if(!quiet) green("Receive:\n");
            printf("%s", got);
        }else WARNX(_("Nothing received"));
    }
    if(G->rot1angle > -999.){
        double angle = convangle(G->rot1angle);
        int steps = (int)(100. * angle);
        waitforstop = movemotor(1, 1, steps, "polaroid");
    }
    if(G->rot2angle > -999.){
        double angle = convangle(G->rot2angle);
        int steps = (int)(80. * angle);
        waitforstop = movemotor(2, 1, steps, "lambda/4");
    }
    if(G->l1steps != INT_MAX){
        waitforstop = movemotor(1, 0, G->l1steps, "polaroid stage");
    }
    if(G->l2steps != INT_MAX){
        waitforstop = movemotor(2, 0, G->l2steps, "lambda/4 stage");
    }
    if((waitforstop && !G->dontwait) || G->waitold) tty_wait();
    if(G->getstatus) tty_getstatus();
    signals(0);
}
