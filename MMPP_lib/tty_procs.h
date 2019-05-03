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
#ifndef TTY_PROCS_H__
#define TTY_PROCS_H__

#include "common.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>        // tcsetattr, baudrates

// tty Rx static buffer
#define TBUFLEN         (1024)
// read timeout (in seconds)
#define TTYTIMEOUT      (0.05)

typedef struct {
    char *portname;         // device filename (should be freed before structure freeing)
    int speed;              // baudrate in human-readable format
    tcflag_t baudrate;      // baudrate (B...)
    struct termios oldtty;  // TTY flags for previous port settings
    struct termios tty;     // TTY flags for current settings
    int comfd;              // TTY file descriptor
    char *buf;              // buffer for data read
    size_t bufsz;           // size of buf
    size_t buflen;          // length of data read into buf
    bool exclusive;         // should device be exclusive opened
} TTYdescr;

int str2double(double *num, const char *str);
double dtime();
TTYdescr *new_tty(char *comdev, int speed, size_t bufsz);
int tty_init(TTYdescr *descr);
TTYdescr *tty_open(TTYdescr *d, bool exclusive);
size_t read_tty(TTYdescr *d);
int write_tty(int comfd, const char *buff, size_t length);
void close_tty(TTYdescr **descr);
int handshake();
char *keyval(char *key, char *haystack);
char *cpy2buf(char *string, size_t *l);

#endif // TTY_PROCS_H__
