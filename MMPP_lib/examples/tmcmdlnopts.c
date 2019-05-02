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
#include <assert.h>
#include <libmmpp.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/*
 * here are global parameters initialisation
 */
static int help;
static glob_pars  G;

int quiet = 0; // less messages @ stdout

//            DEFAULTS
// default global parameters
glob_pars const Gdefault = {
     .comdev = "/dev/ttyUSB0"
    ,.pidfile = "/tmp/MMPP_con.pid"
    ,.speed = BAUD_RATE
};

/*
 * Define command line options by filling structure:
 *  name    has_arg flag    val     type        argptr          help
*/
static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   'h',    arg_none,   APTR(&help),        N_("show this help")},
    {"quiet",   NO_ARGS,    NULL,   'q',    arg_none,   APTR(&quiet),       N_("don't show anything @screen from stdout")},
    {"comdev",  NEED_ARG,   NULL,   'd',    arg_string, APTR(&G.comdev),    N_("terminal device filename")},
    {"baudrate",NEED_ARG,   NULL,   'b',    arg_int,    APTR(&G.speed),     N_("TTY baudrate")},
    {"pidfile", NEED_ARG,   NULL,   'p',    arg_string, APTR(&G.pidfile),   N_("PID-file name")},
    {"stopall", NO_ARGS,    NULL,   's',    arg_none,   APTR(&G.stopall),   N_("stop all motors")},
    {"gettemp", NO_ARGS,    NULL,   't',    arg_none,   APTR(&G.gettemp),   N_("get MCU temperature")},
    {"status",  NO_ARGS,    NULL,   'S',    arg_none,   APTR(&G.getstatus), N_("get device status")},
    end_option
};


/**
 * Parse command line options and return dynamically allocated structure
 *      to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parse_args(int argc, char **argv){
    void *ptr;
    ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
    // format of help: "Usage: progname [args]\n"
    // parse arguments
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(argc > 0){
        WARNX("%d unused parameters:\n", argc);
        for(int i = 0; i < argc; ++i)
            printf("\t%4d: %s\n", i+1, argv[i]);
    }
    return &G;
}

/**
 * @brief MSG show coloured message if `quiet` not set
 *          !! This function adds trailing '\n' to message
 * @param s1 - green part of message (may be null)
 * @param s2 - normal colored part of messate (may be null)
 */
void MSG(const char *s1, const char *s2){
    if(quiet) return;
    if(s1){
        green("%s%s", s1, s2 ? ": " : "\n");
    }
    if(s2) printf("%s\n", s2);
}
