/*
 * tty_procs.h
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
#pragma once
#ifndef TTY_PROCS_H__
#define TTY_PROCS_H__

#include <limits.h>

// amount of tries to establish handshake
#define HANDSHAKE_TRIES     (10)
// steps per full revolution for both rotation stages (1 - polaroid, 2 - waveplate)
#define STEPSREV1           (36000)
#define STEPSREV2           (28800)

typedef enum{
    SEND_ERR,
    SEND_ALLOK,
    SEND_ESWITCH,
    SEND_OTHER
} ttysend_status;

int tty_tryopen(char *dev, int spd);
void tty_close();
char* tty_sendraw(char *string);
int tty_wait();
ttysend_status tty_sendcmd(char *cmd);
int tty_showtemp();
int tty_stopall();
void tty_getstatus();
int handshake();
int mot_getpos(int mcu, int motor);
int mot_getesw(int mcu, int motor);
int init_motors();

#endif // TTY_PROCS_H__
