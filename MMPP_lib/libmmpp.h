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
#ifndef LIBMMPP_H__
#define LIBMMPP_H__

#include <stdbool.h>

// default baudrate for communication
#define BAUD_RATE           (9600)
// amount of tries to establish handshake
#define HANDSHAKE_TRIES     (10)
// how many fails can we omit when waiting for moving end
#define FAIL_TRIES          (5)
// steps per full revolution for both rotation stages (1 - polaroid, 2 - waveplate)
#define STEPSREV1           (36000)
#define STEPSREV2           (28800)

/*
 * MMPP controllers state
 */
// motors:
typedef enum{
    STP_UNKNOWN,    // wrong state
    STP_SLEEP,      // don't moving
    STP_ACCEL,      // start moving with acceleration
    STP_MOVE,       // moving with constant speed
    STP_MVSLOW,     // moving with slowest constant speed
    STP_DECEL,      // moving with deceleration
    STP_STOP,       // stop motor right now (by demand)
    STP_STOPZERO,   // stop motor and zero its position (on end-switch)
    STP_MOVE0,      // move towards 0 endswitch (negative direction)
    STP_MOVE1       // move towards 1 endswitch (positive direction)
} stp_state;
// answers on motor commands
typedef enum{
    SEND_ERR,       // communication error
    SEND_ALLOK,     // no errors
    SEND_ACTIVE,    // motor is still moving
    SEND_TOOBIG,    // amount of steps too big
    SEND_ZEROMOVE,  // give 0 steps to move
    SEND_ESWITCH,   // staying on end-switch & try to move further
    SEND_OTHER
} ttysend_status;
// end-switch state
typedef enum{
    ESW_ERROR,      // wrong value
    ESW_RELEASED,   // opened
    ESW_HALL,       // hall sensor
    ESW_BUTTON      // user button
} ESW_status;
// reset state
typedef enum{
    RESET_NONE,     // no sw/wd reset occured
    RESET_SW,       // software reset have been before last status call
    RESET_WDG       // watchdog reset -//-
} reset_status;
// motor state
typedef struct{
    reset_status rst;           // reset status (was MCU reseted by watchdog or software?)
    stp_state state[2];         // status of stepper motor
    int stepsleft[2];           // steps left to reach target position
    int curpos[2];              // current position (negative for non-initialized state or error)
    ESW_status ESW_status[2][2];// End-switches status, [i][j], i - motor, j - esw 0 or 1
} motor_state;

int mmpp_tryopen(char *dev, int spd);
void mmpp_close();
int mot_handshake();
bool get_rst(int N, bool clear);
bool get_alive(int N);
int stop_all();
int get_temp(double *t1, double *t2);
bool mot_getstatus(int Nmcu, motor_state *s);
int init_motors();
int mot_wait();
ttysend_status tty_sendcmd(char *cmd);

char *tty_get();
int tty_send(char *cmd);
char* tty_sendraw(char *string);

/************************************************************************************
 *                               Wheels management                                  *
 ************************************************************************************/
#define WHEEL_VID "10c4"
#define WHEEL_PID "82cd"

// wheel descriptor
typedef struct{
    int fd;         // file descriptor of device
    char *serial;   // serial number
    char ID;        // identificator
    char name[9];   // wheel name
    int maxpos;     // max position
} wheel_descr;

typedef enum{
     WHERR_ALLOK    // no errors
    ,WHERR_UDEV     // udev error
    ,WHERR_CANTOPEN // can't open file device
} wheel_error;

int find_wheels(wheel_descr **wheels, wheel_error *err);
void del_wheels(wheel_descr *w, int N);

// return value of polling wheel status (if > -1 - current position)
typedef enum{
     WHEEL_MOVING = -2  // still moving
    ,WHEEL_ERROR  = -1  // communication error
    ,WHEEL_POSERR = 0   // wrong wheel position
    ,WHEEL_POS1, WHEEL_POS2, WHEEL_POS3, WHEEL_POS4, WHEEL_POS5, WHEEL_POS6
    ,WHEEL_POS7, WHEEL_POS8, WHEEL_POS9, WHEEL_POS10
} wheel_status;

wheel_status wheel_getpos(wheel_descr *w);
bool wheel_clear_err(wheel_descr *w);
bool move_wheel(wheel_descr *w, int filter_pos);
bool wheel_home(wheel_descr *w);

#endif // LIBMMPP_H__
