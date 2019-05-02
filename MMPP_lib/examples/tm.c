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

#include "tmcmdlnopts.h"
#include <signal.h>
#include <stdio.h>
#include <libmmpp.h>
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
    mmpp_close();
    exit(sig);
}

int main(int argc, char **argv){
    initial_setup();
    signal(SIGTERM, signals);   // kill (-15)
    signal(SIGINT, signals);    // ctrl+C
    signal(SIGQUIT, SIG_IGN);   // ctrl+\   .
    signal(SIGTSTP, SIG_IGN);   // ctrl+Z
    setbuf(stdout, NULL);
    G = parse_args(argc, argv);
    check4running(NULL, G->pidfile);
    DBG("Try to open serial %s", G->comdev);
    if(mmpp_tryopen(G->comdev, G->speed)){
        ERRX(_("Can't open %s with speed %d. Exit."), G->comdev, G->speed);
    }
    if(mot_handshake()){
        WARNX(_("Controllers not found"));
        signals(RET_NOTFOUND);
    }
    for(int i = 1; i < 3; ++i){
        if(get_alive(i)){
            green("MCU #%d found\n", i);
            if(get_rst(i, true))
                red("Controller #%d was in reset state\n", i);
        }else red("MCU #%d not found\n", i);
    }
    if(G->stopall){
        int r = stop_all();
        if(r) red("Error for %d motors of 4\n", r);
        else green("Successfully send command to stop all\n");
    }
    if(G->gettemp){
        double t1, t2;
        if(get_temp(&t1, &t2)){
            green("Got MCU temp:\n");
            if(t1 > -300.) printf("\tMCU#1 - %gdegrC\n", t1);
            if(t2 > -300.) printf("\tMCU#2 - %gdegrC\n", t2);
        }else red("Can't get MCU temp\n");
    }
    if(G->getstatus){
        for(int N = 1; N < 3; ++N){
            motor_state s;
            if(mot_getstatus(N, &s)){
                green("MCU#%d state:\n", N);
                for(int i = 0; i < 2; ++i){
                    printf("\tstate[%d]=%d\n",i, s.state[i]);
                    printf("\tstepslefs[%d]=%d\n", i, s.stepsleft[i]);
                    printf("\tcurpos[%d]=%d\n", i, s.curpos[i]);
                    for(int j = 0; j < 2; ++j)
                        printf("\tESW_status[%d][%d]=%d\n", i, j, s.ESW_status[i][j]);
                }
            }
        }
    }
    int ini = init_motors();
    if(ini) red("Can't init motors: %d\n", ini);
    else green("Motors are ready!\n");
    signals(0);
}
