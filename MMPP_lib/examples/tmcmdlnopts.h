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
#pragma once
#ifndef TMCMDLNOPTS_H__
#define TMCMDLNOPTS_H__

#include <usefull_macros.h>

/*
 * here are some typedef's for global data
 */
typedef struct{
    char *comdev;       // TTY device
    char *pidfile;      // pid file name
    int gettemp;        // get MCU temperature
    int stopall;        // stop all motors
    int speed;          // TTY speed
    int getstatus;      // get status of all devices
    char **sendraw;     // send raw command[s]
    int getADC;         // get ADC values
    double rot1angle;   // rotator 1 angle
    double rot2angle;   // rotator 2 angle
    int l1steps;        // move linear stage 1 (polaroid) for N steps
    int l2steps;        // move linear stage 2 (L/4) for N steps
    int absmove;        // absolute move (to given position from zero-esw)
    int **reset;        // reset given MCU's
} glob_pars;

// default & global parameters
extern glob_pars const Gdefault;
extern int quiet;

glob_pars *parse_args(int argc, char **argv);
void MSG(const char *s1, const char *s2);

#endif // TMCMDLNOPTS_H__
