/*
 * main.c
 *
 * Copyright 2017 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "stm32f0.h"
#include "usart.h"
#include "adc.h"
#include "flash.h"
#include "proto.h"

volatile uint32_t Tms = 0;

/* Called when systick fires */
void sys_tick_handler(void){
    ++Tms;
}

/*
 * PA4 (Tim14Ch1) M1STEP
 * PA6 (Tim3Ch1)  M2STEP
 * PA5 - M2EN
 * PA7 - M2DIR
 * PB1 - 12V on/off
 * PF0 - M1EN
 * PF1 - M1DIR
 */
static void gpio_setup(void){
    // Enable clocks to the GPIO subsystems
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOFEN;
    // PA0..3, PA13, PA14 - AIN; PA4..7 - PUPD;
    GPIOA->MODER = GPIO_MODER_MODER0_AI | GPIO_MODER_MODER1_AI | GPIO_MODER_MODER2_AI |
                GPIO_MODER_MODER3_AI | GPIO_MODER_MODER13_AI | GPIO_MODER_MODER14_AI  |
                GPIO_MODER_MODER4_O | GPIO_MODER_MODER5_O | GPIO_MODER_MODER6_O | GPIO_MODER_MODER7_O;
    GPIOA->OSPEEDR = 0; // all low speed
    GPIOA->PUPDR = 0; // clear pull-down for PA14&PA13
    // PA4 - Tim14Ch1 (AF4), PA6 - Tim3Ch1 (AF1)
    GPIOA->AFR[0] = (GPIOA->AFR[0] &~ (GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL6))\
                | (4 << (4 * 4)) | (1 << (6 * 4));
    // PB1 - PUPD
    GPIOB->MODER = GPIO_MODER_MODER1_O;
    // PF0, PF1 - PUPD
    GPIOF->MODER = GPIO_MODER_MODER0_O | GPIO_MODER_MODER1_O;
}

int main(void){
    uint32_t lastT = 0;
    uint32_t ostctr = 0;
    #if 0
    //def EBUG
    uint32_t msgctr = 0;
    #endif
    char *txt = NULL;
    sysreset();
    SysTick_Config(6000, 1);
    gpio_setup();
    adc_setup();
    USART1_config();
    get_userconf();
    //pin_set(GPIOA, 1<<5); // clear extern LED
    while (1){
        if(lastT > Tms || Tms - lastT > 499){
            #ifdef EBUG
            pin_toggle(GPIOA, 1<<4); // blink by onboard LED once per second
            #endif
            lastT = Tms;
        }
        if(usart1rx()){ // usart1 received data, store in in buffer
            if(usart1_getline(&txt)){
                txt = process_command(txt);
            }else txt = NULL; // buffer overflow
        }
        #if 0
        //def EBUG
        if(msgctr > Tms || Tms - msgctr > 4999){ // once per 5 seconds
            msgctr = Tms;
            txt = "hello";
        }
        #endif
        if(trbufisfull()){
            write2trbuf("ERR");
            usart1_send_blocking(gettrbuf());
        }
        cleartrbuf();
        if(txt){ // text waits for sending
            if(ALL_OK == usart1_send(txt)){
                txt = NULL;
            }
        }
        if(ostctr != Tms){
            ostctr = Tms;
            //
        }
    }
}

