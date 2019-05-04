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

/******************************************************************************
 *                         Wheels testing tool                                *
 ******************************************************************************/

#include "wheelscmdlnopts.h"
#include <signal.h>
#include <stdio.h>
#include <libmmpp.h>
#include <usefull_macros.h>

// All return states of main():
enum{
    RET_ALLOK = 0,
    RET_NOTFOUND,       // none of turrets found or didn't found seeking MCU
    RET_ONLYONE,        // only one turret found
    RET_COMMERR,        // communication error
    RET_CANTINIT,       // can't init turrets
    RET_ERROR = 9,      // uncoverable error - from libsnippets
    RET_HELPCALL = 255  // user call help (or give wrong parameter[s]) - from libsnippets
};
static glob_pars *G;
static wheel_descr *wheels;
static int found;

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
    del_wheels(wheels, found);
    exit(sig);
}

/**
 * @brief find_wheel_by_ID - find wheel by ID
 * @param w  - array of wheel descriptors
 * @param N  - length of w
 * @param ID - sought wheel ID
 * @return NULL if no such wheel or pointer to it
 */
wheel_descr *find_wheel_by_ID(wheel_descr *w, int N, char ID){
    for(int i = 0; i < N; ++i){
        if(w[i].ID == ID) return &w[i];
    }
    return NULL;
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
    wheel_error werr;
    found = find_wheels(&wheels, &werr);
    if(found == 0) ERRX(_("No wheels found"));
    else green("Found %d wheels\n", found);
    if(werr != WHERR_ALLOK){
        WARNX(_("Got wheel error: %d"), werr);
        signals(1);
    }
    if(G->getstatus){
        for(int i = 0; i < found; ++i){
            green("Wheel #%d:\n", i);
            printf("\tserial: %s\n", wheels[i].serial);
            printf("\tID: %c\n", wheels[i].ID);
            printf("\tname: %s\n", wheels[i].name);
            printf("\tmaxpos: %d\n\n", wheels[i].maxpos);
        }
    }
    if(G->gohome){ // non-blocking moving to home position
        for(int i = 0; i < found; ++i){
            if(!wheel_home(&wheels[i])) WARNX(_("Can't move wheel %c to home position"), wheels[i].ID);
            else{
                green("Wheel %c is moving to home position\n");
            }
        }
        // now we can wait until all wheels reach home position
        for(int i = 0; i < found; ++i){
            while(WHEEL_MOVING == wheel_getpos(&wheels[i])){
                usleep(100000);
            }
            // we should check current position because wheel can be blocked and don't move
            if(wheel_getpos(&wheels[i]) != 1)
                WARNX(_("Wheel %c didn't reach home position"), wheels[i].ID);
        }
    }
    int Nw = 0, Ng = 0;
    if(G->wh_ids){ // count arguments of --wheel-id
        while(G->wh_ids[Nw]) ++Nw;
    }
    if(G->gotopos){ // count arguments of --goto
        while(G->gotopos[Ng]) ++Ng;
    }
    if(Nw != Ng){ // it's better to write --goto after each --wheel-id
        WARNX(_("Amoung of `--wheel-id` should be equal to amount of `--goto`!"));
    }else{ // here is an example of searching wheel by its ID and blocking moving
        for(int i = 0; i < Nw; ++i){
            char ID = *G->wh_ids[i];
            DBG("id: %c, goto: %d", ID, *G->gotopos[i]);
            wheel_descr *w = find_wheel_by_ID(wheels, found, ID);
            if(!w) WARNX(_("No wheel with ID %c found!"), ID);
            else{
                int pos = *G->gotopos[i];
                if(!move_wheel(w, pos)){
                    WARNX(_("Can't rotate wheel %c to position %d"), ID, pos);
                    wheel_clear_err(w);
                }else{ // wait until wheel is moving
                    while(WHEEL_MOVING == wheel_getpos(w)){
                        DBG("still moving");
                        usleep(100000);
                    }
                    int curpos = wheel_getpos(w); // poll again to check current position
                    if(curpos != pos) WARNX(_("Wheel %c can't reach position %d, current position: %d"), ID, pos, curpos);
                    else green("Wheel %c is on position %d\n", ID, pos);
                }
            }
        };
    }
    ;
    signals(0);
}
