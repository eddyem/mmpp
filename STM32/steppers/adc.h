/*
 *                                                                                                  geany_encoding=koi8-r
 * adc.h
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
#ifndef __ADC_H__
#define __ADC_H__

// 8 channels (including inttemp & vrefint)
#define NUMBER_OF_ADC_CHANNELS (6)

extern uint16_t ADC_array[];
void adc_setup();

typedef enum{
    ESW_RELEASED,
    ESW_HALL,
    ESW_BUTTON,
    ESW_ERROR
} ESW_status;

int32_t getTemp();
uint32_t getVdd();
uint32_t getVmot();
uint32_t getImot();
ESW_status eswStatus(int motnum, int eswnum);

#endif // __ADC_H__
