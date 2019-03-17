/*
 * cmdlnopts.c - the only function that parse cmdln args and returns glob parameters
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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
 */

#include "cmdlnopts.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
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
     .showtemp = 0
    ,.comdev = "/dev/ttyUSB0"
    ,.pidfile = "/tmp/MMPP_control.pid"
    ,.speed = BAUD_RATE
    ,.rot1angle = -1000.
    ,.rot2angle = -1000.
    ,.l1steps = INT_MAX
    ,.l2steps = INT_MAX
};

/*
 * Define command line options by filling structure:
 *  name    has_arg flag    val     type        argptr          help
*/
static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   'h',    arg_none,   APTR(&help),        N_("show this help")},
    {"quiet",   NO_ARGS,    NULL,   'q',    arg_none,   APTR(&quiet),       N_("don't show anything @screen from stdout")},
    {"absmove", NO_ARGS,    NULL,   'A',    arg_none,   APTR(&G.absmove),   N_("absolute move (without this flag moving is relative)")},
    {"temp",    NO_ARGS,    NULL,   't',    arg_none,   APTR(&G.showtemp),  N_("show temperature of both MCU")},
    {"comdev",  NEED_ARG,   NULL,   'd',    arg_string, APTR(&G.comdev),    N_("terminal device filename")},
    {"sendraw", NEED_ARG,   NULL,   'a',    arg_string, APTR(&G.sendraw),   N_("send RAW string to port and read the answer")},
    {"reset",   MULT_PAR,   NULL,   'E',    arg_int,    APTR(&G.reset),     N_("reset given mcu (may be included several times)")},
    {"rot1",    NEED_ARG,   NULL,   'R',    arg_double, APTR(&G.rot1angle), N_("rotate polaroid to given angle")},
    {"rot2",    NEED_ARG,   NULL,   'r',    arg_double, APTR(&G.rot2angle), N_("rotate lambda/4 to given angle")},
    {"status",  NO_ARGS,    NULL,   's',    arg_none,   APTR(&G.getstatus), N_("get device status")},
    {"baudrate",NEED_ARG,   NULL,   'b',    arg_int,    APTR(&G.speed),     N_("TTY baudrate")},
    {"wait",    NO_ARGS,    NULL,   'w',    arg_none,   APTR(&G.waitold),   N_("wait while all previous moving ends")},
    {"async",   NO_ARGS,    NULL,   'y',    arg_none,   APTR(&G.dontwait),  N_("asynchronous moving - don't wait")},
    {"lin1",    NEED_ARG,   NULL,   'L',    arg_int,    APTR(&G.l1steps),   N_("move polaroid linear stage to N steps")},
    {"lin2",    NEED_ARG,   NULL,   'l',    arg_int,    APTR(&G.l2steps),   N_("move waveplate linear stage to N steps")},
    {"pidfile", NEED_ARG,   NULL,   'p',    arg_string, APTR(&G.pidfile),   N_("PID-file name")},
    {"stop",    NO_ARGS,    NULL,   'S',    arg_none,   APTR(&G.stopall),   N_("stop any moving")},
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
