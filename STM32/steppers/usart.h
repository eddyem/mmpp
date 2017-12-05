/*
 * usart.h
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
 */
#pragma once
#ifndef __USART_H__
#define __USART_H__

#include "stm32f0.h"

// input and output buffers size
#define UARTBUFSZ  (64)
// timeout between data bytes
#define TIMEOUT_MS (1500)
// check timeout
#define CHECK_TMOUT

typedef enum{
    ALL_OK,
    LINE_BUSY,
    STR_TOO_LONG
} TXstatus;

#define usart1rx()  (linerdy)
#define usart1ovr() (bufovr)

extern int linerdy, bufovr, txrdy;
extern int trbufidx;

void USART1_config();
int usart1_getline(char **line);
TXstatus usart1_send(char *str);
#define usart1_send_blocking(str) do{}while(LINE_BUSY == usart1_send(str))

#define cleartrbuf()  do{trbufidx = 0;}while(0)
#define trbufisfull()  (trbufidx)
int put2trbuf(char c);
int write2trbuf(const char *str);
char *gettrbuf();
int put_int(int32_t N);
int put_uint(uint32_t N);


#endif // __USART_H__
