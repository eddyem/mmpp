/*
 *                                                                                                  geany_encoding=koi8-r
 * steppers.h
 *
 * Copyright 2017 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#pragma once
#ifndef __STEPPERS_H__
#define __STEPPERS_H__

#include "stm32f0.h"

extern int32_t mot_position[];
extern uint32_t steps_left[];

// stepper states
typedef enum{
    STP_SLEEP,      // don't moving
    STP_ACCEL,      // start moving with acceleration
    STP_MOVE,       // moving with constant speed
    STP_MVSLOW,     // moving with slowest constant speed
    STP_DECEL,      // moving with deceleration
    STP_STOP,       // stop motor right now (by demand)
    STP_STOPZERO,   // stop motor and zero its position (on end-switch)
    STP_MOVE0,      // move towards 0 endswitch (negative direction)
    STP_MOVE1,      // move towards 1 endswitch (positive direction)
} stp_state;

typedef enum{
    STPS_ALLOK,     // no errors
    STPS_ACTIVE,    // motor is still moving
    STPS_TOOBIG,    // amount of steps too big
    STPS_ZEROMOVE,  // give 0 steps to move
    STPS_ONESW      // staying on end-switch & try to move further
} stp_status;

#define stp_position(n)     (mot_position[n])
#define stp_stepsleft(n)    (steps_left[n])

int stp_isactive(int motnum);
void stp_setup();
void stp_chspd();
stp_status stp_move(int nmotor, int32_t steps);
void stp_stop(int n);
void stp_process();

#endif // __STEPPERS_H__
