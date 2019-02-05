/*
 *                                                                                                  geany_encoding=koi8-r
 * steppers.c
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

#include "steppers.h"
#include "flash.h"
#include "adc.h"
#include "usart.h"

// amount of steps need for full acceleration/deceleration cycle
#define ACCDECSTEPS (the_conf.accdecsteps)
// amount of microsteps in each step
#define USTEPS      (the_conf.usteps)

static GPIO_TypeDef* const MPORT[2]   = {GPIOF, GPIOA};
static const uint16_t MENPIN[2]  = { 1<<0, 1 << 5}; // enable pins: PF0 and PA5
static const uint16_t MDIRPIN[2] = { 1<<1, 1 << 7}; // direction pins: PF1 and PA7

int32_t mot_position[2] = {-1, -1}; // current position of motor (from zero endswitch, -1 means inactive)
uint32_t steps_left[2] = {0,0}; // amount of steps left
static stp_state state[2] = {STP_SLEEP, STP_SLEEP}; // current state of motor
// ARR register values: low (max speed), high (min speed = 10% from max), step (1/50(hi-lo))
static uint16_t stplowarr[2], stphigharr[2], stpsteparr[2];
static int8_t dir[2] = {0,0}; // moving direction: -1 (negative) or 1 (positive)

// return 1 if motor is in active state
stp_state stp_getstate(int motnum){
    return state[motnum];
}

// turn ~EN to 1 for both motors
void stp_disable(){
    pin_set(MPORT[0], MENPIN[0]);
    pin_set(MPORT[1], MENPIN[1]);
}

void stp_chspd(){
    int i;
    for(i = 0; i < 2; ++i){
        uint16_t spd = the_conf.motspd[i];
        stplowarr[i] = spd;
        stphigharr[i] = spd * LOWEST_SPEED_DIV;
        stpsteparr[i] = (spd * (LOWEST_SPEED_DIV - 1)) / ((uint16_t)ACCDECSTEPS) + 1;
    }
}

// Tim3_ch1 - PA6, Tim14ch1 - PA4; 48MHz -> 48kHz
static void timers_setup(){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM14EN; // enable clocking
    //TIM3->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0; // PWM mode 2: inacive->active, preload enable
    TIM3->CCMR1 = TIM_CCMR1_OC1M_2; // Force inactive
    TIM3->PSC = 999; // 48kHz
    //TIM3->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E; // turn it on, active low
    TIM3->CCER = TIM_CCER_CC1E; // turn it on, active high
    TIM3->CCR1 = 1; // 20.8us for pulse duration, according to datasheet 1.9us is enough
    TIM3->ARR = 1000; // starting ARR value
    //TIM14->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;
    TIM14->CCMR1 = TIM_CCMR1_OC1M_2;
    TIM14->PSC = 999;
    //TIM14->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E;
    TIM14->CCER = TIM_CCER_CC1E;
    TIM14->CCR1 = 1;
    TIM14->ARR = 1000;
    // enable IRQ & update values
    TIM3->EGR = TIM_EGR_UG;
    TIM14->EGR = TIM_EGR_UG;
    TIM3->DIER = TIM_DIER_CC1IE; // allow CC interrupt (we should count steps)
    TIM14->DIER = TIM_DIER_CC1IE;
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, 0);
    NVIC_EnableIRQ(TIM14_IRQn);
    NVIC_SetPriority(TIM14_IRQn, 0);
}

// setup timers
void stp_setup(){
    stp_chspd();
    timers_setup();
}

// check end-switches for stepper motors
void stp_process(){
    static uint16_t lastbtnpressed = 0; // anticlash counter
    int i = 0;
    ESW_status esw[2];
    // check ESW0 for buttons
    esw[0] = eswStatus(0, 0);
    esw[1] = eswStatus(0, 1);
    if(esw[0] == ESW_BUTTON || esw[1] == ESW_BUTTON){
        if(lastbtnpressed++ == 5){ // stop all motors @ button
            uint8_t stopped = 0;
            if(state[0] != STP_SLEEP){
                state[0] = STP_STOP;
                stopped = 1;
            }
            if(state[1] != STP_SLEEP){
                state[1] = STP_STOP;
                stopped = 1;
            }
            if(stopped){
                DBG("S(btn)\n");
                lastbtnpressed = 51; // override value
            }
        }else if(lastbtnpressed == 50){ // one or both buttons pressed, run only after ~100ms
            if(esw[0] == ESW_BUTTON && esw[1] == ESW_BUTTON){
                // both buttons pressed - move MOTOR1 to zero
                state[1] = STP_MOVE0;
            }else{ // move motor 0 to 0 or 1
                if(esw[0] == ESW_BUTTON) state[0] = STP_MOVE0;
                else state[0] = STP_MOVE1;
            }
        }
    }else lastbtnpressed = 0;

    for(i = 0; i < 2; ++i){ // check motors' status
        stp_state curst = state[i];
        esw[0] = eswStatus(i, 0);
        esw[1] = eswStatus(i, 1);
        switch(curst){
            case STP_MOVE0: // move towards zero endswitch
                state[i] = STP_SLEEP;
                stp_move(i, -the_conf.maxsteps[i]); // won't move if the_conf.maxsteps == 0
            break;
            case STP_MOVE1:
                state[i] = STP_SLEEP;
                stp_move(i, the_conf.maxsteps[i]);
            break;
            case STP_ACCEL: // @ any move check esw
            case STP_DECEL:
            case STP_MOVE:
            case STP_MVSLOW: // check end-switches status
                if(esw[0] == ESW_HALL && dir[i] == -1){
                    DBG("S@0\n");
                    state[i] = STP_STOPZERO; // stop @ end-switch
                }else{ if(esw[1] == ESW_HALL && dir[i] ==  1){
                    state[i] = STP_STOP; // stop @ end-switch 1
                    DBG("S@1\n");
                    }
                }
            break;
            default: // stopping states - do nothing
            break;
        }
    }
}

// move motor `nmotor` to `steps` steps, @return 0 if all OK
stp_status stp_move(int nmotor, int32_t steps){
    stp_state st = state[nmotor];
    if(st != STP_SLEEP && st != STP_MOVE0 && st != STP_MOVE1) return STPS_ACTIVE;
    if(steps == 0)
        return STPS_ZEROMOVE;
    if(the_conf.maxsteps[nmotor] && steps > the_conf.maxsteps[nmotor]) return STPS_TOOBIG;
    int8_t d;
    if(steps < 0){
        d = -1;
        steps = -steps;
    }else d = 1; // positive direction
    // check end-switches
    if(eswStatus(nmotor, 0) == ESW_HALL && d == -1) return STPS_ONESW;
    if(eswStatus(nmotor, 1) == ESW_HALL && d == 1)  return STPS_ONESW;
    dir[nmotor] = d;
    // change value of DIR pin
    if(the_conf.reverse[nmotor]){
        if(d>0)
            pin_set(MPORT[nmotor], MDIRPIN[nmotor]);
        else
            pin_clear(MPORT[nmotor], MDIRPIN[nmotor]);
    }else{
        if(d>0)
            pin_clear(MPORT[nmotor], MDIRPIN[nmotor]);
        else
            pin_set(MPORT[nmotor], MDIRPIN[nmotor]);
    }
    // turn on EN pin (0)
    pin_clear(MPORT[nmotor], MENPIN[nmotor]);
    steps_left[nmotor] = (uint32_t)steps;
    // setup timer & start it
    TIM_TypeDef *TIMx = nmotor ? TIM3 : TIM14;
    TIMx->ARR = stphigharr[nmotor];
    TIMx->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1: active->inacive, preload enable
    TIMx->CR1 |= TIM_CR1_CEN;
    if(steps < ACCDECSTEPS*2) state[nmotor] = STP_MVSLOW; // move without acceleration
    else state[nmotor] = STP_ACCEL; // move with acceleration
    return STPS_ALLOK;
}

// change ARR and for given stepper
void stp_chARR(int n, int32_t val){
    TIM_TypeDef *TIMx = n ? TIM3 : TIM14;
    if(val < 2) val = 2;
    TIMx->ARR = (uint32_t)val;
}

void stp_stop(int n){ // stop motor by demand or @ end-switch
    switch(state[n]){
        case STP_SLEEP:
            return;
        break;
        case STP_MOVE0:
        case STP_MOVE1:
            state[n] = STP_SLEEP;
        break;
        default:
            state[n] = STP_STOP;
    }
}


// timer interrupt
static void stpr_int(int n){
    static uint8_t ustep[2] = {0,0};
    TIM_TypeDef *TIMx = n ? TIM3 : TIM14;
    uint16_t tmp, arrval;
    if(USTEPS == ++ustep[n]){ // prevent stop @ not full step
        ustep[n] = 0;
        if(state[n] == STP_STOPZERO)
            mot_position[n] = 0;
        else{
            if(0 == --steps_left[n]) state[n] = STP_STOP;
            mot_position[n] += dir[n];
        }
    }else return;
    switch(state[n]){
        case STP_ACCEL: // acceleration phase
            arrval = (uint16_t)TIMx->ARR - stpsteparr[n];
            tmp = stplowarr[n];
            if(arrval <= tmp || arrval > stphigharr[n]){
                arrval = tmp;
                state[n] = STP_MOVE; // end of acceleration phase
                DBG("AccEnd\n");
            }
            TIMx->ARR = arrval;
        break;
        case STP_DECEL: // deceleration phase
            arrval = (uint16_t)TIMx->ARR + stpsteparr[n];
            tmp = stphigharr[n];
            if(arrval >= tmp || arrval < stplowarr[n]){
                arrval = tmp;
                state[n] = STP_MVSLOW; // end of deceleration phase, move @ lowest speed
                DBG("DecEnd\n");
            }
            TIMx->ARR = arrval;
        break;
        case STP_MOVE:   // moving with constant speed phases
            if(steps_left[n] <= ACCDECSTEPS){
                state[n] = STP_DECEL; // change moving status to decelerate
                DBG("->Dec\n");
            }
        break;
        case STP_MVSLOW:
            // nothing to do here: all done before switch()
        break;
        default: // STP_STOP, STP_STOPZERO
            ustep[n] = 0;
            TIMx->CCMR1 = TIM_CCMR1_OC1M_2; // Force inactive
            TIMx->CR1 &= ~TIM_CR1_CEN; // stop timer
            pin_set(MPORT[n], MENPIN[n]); // turn off motor power
            dir[n] = 0;
            steps_left[n] = 0;
            state[n] = STP_SLEEP;
            DBG("Stop\n");
        break;
    }
}

// interrupt from stepper 1 timer
void tim3_isr(){
    stpr_int(1);
    TIM3->SR = 0;
}
// interrupt from stepper 0 timer
void tim14_isr(){
    stpr_int(0);
    TIM14->SR = 0;
}
