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
    RET_NOTFOUND,       // none of turrets found or didn't found seeking MCU
    RET_ONLYONE,        // only one turret found
    RET_COMMERR,        // communication error
    RET_CANTINIT,       // can't init turrets
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
    ;
    signals(0);
}
