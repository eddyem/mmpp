/*
 *                                                                                                  geany_encoding=koi8-r
 * adc.c
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
#include "flash.h"
#include "adc.h"
#include "usart.h"

extern volatile uint32_t Tms; // time counter for 1-second Vdd measurement
static uint32_t lastVddtime = 0; // Tms value of last Vdd measurement
static uint32_t VddValue = 0; // value of Vdd * 100 (for more precision measurements)
// check time of last Vdd measurement & refresh it value
#define CHKVDDTIME() do{if(!VddValue || Tms < lastVddtime || Tms - lastVddtime > 999) getVdd();}while(0)

/*
 * 0 - Steppers current
 * 1 - Input voltage 12V
 * 2 - EndSwitch2 of motor1
 * 3 - EndSwitch1 of motor1
 * 4 - inner temperature
 * 5 - vref
 */
uint16_t ADC_array[NUMBER_OF_ADC_CHANNELS];

void adc_setup(){
    // AIN: PA0..3, PA13, PA14. ADC_IN16 - inner temperature. ADC_IN17 - VREFINT
    /* (1) Enable the peripheral clock of the ADC */
    /* (2) Start HSI14 RC oscillator */
    /* (3) Wait HSI14 is ready */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; /* (1) */
    RCC->CR2 |= RCC_CR2_HSI14ON; /* (2) */
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0){}; /* (3) */
    /* (1) Ensure that ADEN = 0 */
    /* (2) Clear ADEN */
    /* (3) Launch the calibration by setting ADCAL */
    /* (4) Wait until ADCAL=0 */
    if ((ADC1->CR & ADC_CR_ADEN) != 0){ /* (1) */
        ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  /* (2) */
    }
    ADC1->CR |= ADC_CR_ADCAL; /* (3) */
    while ((ADC1->CR & ADC_CR_ADCAL) != 0){} /* (4) */
    /* (1) Enable the ADC */
    /* (2) Wait until ADC ready */
    do{
        ADC1->CR |= ADC_CR_ADEN; /* (1) */
    }while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); /* (2) */;
    /* (1) Select HSI14 by writing 00 in CKMODE (reset value) */
    /* (2) Select the continuous mode */
    /* (3) Select CHSEL0..3, 16,17 */
    /* (4) Select a sampling mode of 111 i.e. 239.5 ADC clk to be greater than 17.1us */
    /* (5) Wake-up the VREFINT and Temperature sensor (only for VBAT, Temp sensor and VRefInt) */
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_CONT; /* (2)*/
    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 |
            ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17; /* (3)*/
    ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2; /* (4) */
    ADC->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN; /* (5) */
    // DMA for AIN
    /* (1) Enable the peripheral clock on DMA */
    /* (2) Enable DMA transfer on ADC and circular mode */
    /* (3) Configure the peripheral data register address */
    /* (4) Configure the memory address */
    /* (5) Configure the number of DMA tranfer to be performs on DMA channel 1 */
    /* (6) Configure increment, size, interrupts and circular mode */
    /* (7) Enable DMA Channel 1 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN; /* (1) */
    ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG; /* (2) */
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR)); /* (3) */
    DMA1_Channel1->CMAR = (uint32_t)(ADC_array); /* (4) */
    DMA1_Channel1->CNDTR = NUMBER_OF_ADC_CHANNELS; /* (5) */
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC; /* (6) */
    DMA1_Channel1->CCR |= DMA_CCR_EN; /* (7) */
    ADC1->CR |= ADC_CR_ADSTART; /* start the ADC conversions */
}

// return MCU temperature (degrees of celsius)
int32_t getTemp(){
    CHKVDDTIME();
    // make correction on Vdd value
    int32_t temperature = (int32_t)ADC_array[4] * VddValue / 330;
/*
write2trbuf("getTemp()\ncal30=");
put_uint(*TEMP30_CAL_ADDR);
write2trbuf(", cal110=");
put_uint(*TEMP110_CAL_ADDR);
write2trbuf(", t=");
put_int(temperature);
SENDBUF();
*/
    temperature = (int32_t) *TEMP30_CAL_ADDR - temperature;
/*
put_int(temperature);
SENDBUF();
*/
    temperature *= (int32_t)(1100 - 300);
/*
put_int(temperature);
SENDBUF();
*/
    temperature = temperature / (int32_t)(*TEMP30_CAL_ADDR - *TEMP110_CAL_ADDR);
/*
put_int(temperature);
SENDBUF();
*/
    temperature += 300;
    return(temperature);
}

// return Vdd * 100 (V)
uint32_t getVdd(){
    #define ARRSZ (10)
    static uint16_t arr[ARRSZ] = {0};
    static int arridx = 0;
    uint32_t v = ADC_array[5];
    int i;
/*
write2trbuf("getVdd(), val=");
put_uint(v);
write2trbuf(", cal=");
put_uint(*VREFINT_CAL_ADDR);
SENDBUF();
*/
    if(arr[0] == 0){ // first run - fill all with current data
/*
write2trbuf("1st run");
SENDBUF();
*/
        for(i = 0; i < ARRSZ; ++i) arr[i] = (uint16_t) v;
    }else{
/*
write2trbuf("arridx=");
put_int(arridx);
SENDBUF();
*/
        arr[arridx++] = v;
        v = 0; // now v is mean
        if(arridx > ARRSZ-1) arridx = 0;
        // calculate mean
        for(i = 0; i < ARRSZ; ++i){
/*
write2trbuf("arr["); put2trbuf('0'+i); write2trbuf("]=");
put_uint(arr[i]);
SENDBUF();
*/
            v += arr[i];
        }
        v /= ARRSZ;
/*
write2trbuf("mean value: ");
put_uint(v);
SENDBUF();
*/
    }
    uint32_t vdd = ((uint32_t) *VREFINT_CAL_ADDR) * (uint32_t)330 * the_conf.v33numerator; // 3.3V
/*
put_uint(vdd);
SENDBUF();
*/
    vdd /= v * the_conf.v33denominator;
/*
put_uint(vdd);
SENDBUF();
*/
    lastVddtime = Tms;
    VddValue = vdd;
    return vdd;
}

// return value of 12V * 100 (V)
uint32_t getVmot(){
    CHKVDDTIME();
    uint32_t vmot = ADC_array[1] * VddValue * the_conf.v12numerator;
    vmot >>= 12;
    vmot /= the_conf.v12denominator;
    return vmot;
}

// return value of motors' current * 100 (A)
uint32_t getImot(){
    CHKVDDTIME();
    uint32_t vmot = ADC_array[0] * VddValue * the_conf.i12numerator;
    vmot >>= 12;
    vmot /= the_conf.i12denominator;
    return vmot;
}

// end-switches status: 0 - don't activated, 1 - activated, 2 - user button, 4 - error
// @param motnum - motor number  (0,1)
// @param eswnum - switch number (0,1)
ESW_status eswStatus(int motnum, int eswnum){
    int idx;
    if(motnum){ // motor 1 have no ADC - just 0 or 1
        if(eswnum){ // ESW11 - PA14
            if(GPIOA->IDR & 1<<14) return ESW_RELEASED;
            else return ESW_HALL;
        }else{ // ESW10 - PA13
            if(GPIOA->IDR & 1<<13) return ESW_RELEASED;
            else return ESW_HALL;
        }
    }else{ // motor 0
        if(eswnum) idx = 2;
        else idx = 3;
    }
    uint16_t thres = the_conf.ESW_thres, val = ADC_array[idx];
    // low sighal: 0..threshold - Hall activated
    if(val < thres) return ESW_HALL;
    // high signal: (4096-thres)..4096 - pullup
    if(val > (uint16_t)0x1000 - thres) return ESW_RELEASED;
    // middle signal: 0x800-thres..0x800+thres - user button active (47k pullup + 47k pulldown)
    if(0x800 - thres < val && val < 0x800 + thres) return ESW_BUTTON;
    // very strange, return err
    return ESW_ERROR;
}
