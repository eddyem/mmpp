/*
 *                                                                                                  geany_encoding=koi8-r
 * flash.c
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
#include "stm32f0.h"
#include <string.h> // memcpy

#include "flash.h"
#include "usart.h"

// start of configuration data in flash (from 15kB, one kB size)
#define FLASH_CONF_START_ADDR   ((uint32_t)0x08003C00)

user_conf the_conf = {
     .good_data_pos = 0xffffffff
    ,.devID = 0
    ,.v12numerator = 1
    ,.v12denominator = 1
    ,.i12numerator = 1
    ,.i12denominator = 1
    ,.v33denominator = 1
    ,.v33numerator = 1
    ,.ESW_thres = 150
};

static int maxnum = 0x800 / sizeof(user_conf);

static int erase_flash();

static int get_gooddata(){
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    uint32_t datapos = c->good_data_pos;
    if(datapos == 0xffffffff){ // virginity clear
        return maxnum;
    }
    // have data - move it to `the_conf`
    if(maxnum > 32) maxnum = 32;
    int idx;
    for(idx = 1; idx < maxnum; ++idx){ // find current settings index - first non-zero bit
        if(datapos & 1<<idx){
            break;
        }
    }
    return idx-1;
}

void get_userconf(){
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    int idx = get_gooddata();
    if(idx == maxnum) return;
    memcpy(&the_conf, &c[idx], sizeof(user_conf));
}

// store new configuration
// @return 0 if all OK
int store_userconf(){
char buf[2] = {0,0};
    int ret = 0;
    user_conf *c = (user_conf*) FLASH_CONF_START_ADDR;
    int idx = get_gooddata();
    if(idx == maxnum || idx == maxnum - 1){ // first run or there's no more place
        idx = 0;
        if(erase_flash()) return 1;
    }else ++idx; // take next data position
    if (FLASH->CR & FLASH_CR_LOCK){
        FLASH->KEYR = FLASH_FKEY1;
        FLASH->KEYR = FLASH_FKEY2;
    }
    the_conf.good_data_pos = 0xffffffff ^ (1<<idx); // write zero to corresponding position
    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    FLASH->CR |= FLASH_CR_PG;
    uint16_t *data = (uint16_t*) &the_conf;
    uint16_t *address = (uint16_t*) &c[idx];
    uint32_t i, count = sizeof(user_conf) / 2;
    for (i = 0; i < count; ++i){
        //*(volatile uint16_t*)(address + i) = (((uint8_t)data[i + 1]) << 8) | data[i];
        *(volatile uint16_t*)(address + i) = data[i];
        //while (!(FLASH->SR & FLASH_SR_EOP));
        while((FLASH->SR & FLASH_SR_BSY));
buf[0] = '0' + i;
usart1_send_blocking(buf);
        if(FLASH->SR &  FLASH_SR_PGERR) ret = 1;
buf[0] = ret + '0';
usart1_send_blocking(buf);
        FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
    }
    FLASH->CR &= ~(FLASH_CR_PG);
    return ret;
}


static int erase_flash(){
    int ret = 0;
    /* (1) Wait till no operation is on going */
    /* (2) Clear error & EOP bits */
    /* (3) Check that the Flash is unlocked */
    /* (4) Perform unlock sequence */
    while ((FLASH->SR & FLASH_SR_BSY) != 0){} /* (1) */
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;  /* (2) */
  /*  if (FLASH->SR & FLASH_SR_EOP){
        FLASH->SR |= FLASH_SR_EOP;
    }*/
    if ((FLASH->CR & FLASH_CR_LOCK) != 0){ /* (3) */
        FLASH->KEYR = FLASH_FKEY1; /* (4) */
        FLASH->KEYR = FLASH_FKEY2;
    }
    /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
    /* (2) Program the FLASH_AR register to select a page to erase */
    /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
    /* (4) Wait until the  EOP flag in the FLASH_SR register set */
    /* (5) Clear EOP flag by software by writing EOP at 1 */
    /* (6) Reset the PER Bit to disable the page erase */
    FLASH->CR |= FLASH_CR_PER; /* (1) */
    FLASH->AR = FLASH_CONF_START_ADDR; /* (2) */
    FLASH->CR |= FLASH_CR_STRT; /* (3) */
    while(!(FLASH->SR & FLASH_SR_EOP));
    FLASH->SR |= FLASH_SR_EOP; /* (5)*/
    if(FLASH->SR & FLASH_SR_WRPERR){ /* Check Write protection error */
        ret = 1;
        FLASH->SR |= FLASH_SR_WRPERR; /* Clear the flag by software by writing it at 1*/
    }
    FLASH->CR &= ~FLASH_CR_PER; /* (6) */
    return ret;
}

