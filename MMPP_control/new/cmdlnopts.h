/*
 * cmdlnopts.h - comand line options for parceargs
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

#pragma once
#ifndef CMDLNOPTS_H__
#define CMDLNOPTS_H__

#include <stdint.h>
#include <usefull_macros.h>

/*
 * here are some typedef's for global data
 */
typedef struct{
    int showtemp;       // show temperatures of both MCU
    char *comdev;       // TTY device
    char *sendraw;      // send RAW string
    char *pidfile;      // pid file name
    double rot1angle;   // rotator 1 angle
    double rot2angle;   // rotator 2 angle
    int speed;          // TTY speed
    int getstatus;      // get both MCU status
    int waitold;        // wait for previous moving ends
    int dontwait;       // don't wait for moving end
    int l1steps;        // move linear stage 1 (polaroid) for N steps
    int l2steps;        // move linear stage 2 (L/4) for N steps
    int absmove;        // absolute move (to given position from zero-esw)
    int stopall;        // stop all moving
    int **reset;        // reset given MCU's
} glob_pars;

// default & global parameters
extern glob_pars const Gdefault;
extern int quiet;

glob_pars *parse_args(int argc, char **argv);
void MSG(const char *s1, const char *s2);

#endif // CMDLNOPTS_H__
