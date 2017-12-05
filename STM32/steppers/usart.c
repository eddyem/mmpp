/*
 * usart.c
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

#include "usart.h"
#include <string.h> // memcpy

extern volatile uint32_t Tms;

static int datalen[2] = {0,0}; // received data line length (including '\n')

int linerdy = 0,        // received data ready
    dlen = 0,           // length of data (including '\n') in current buffer
    bufovr = 0,         // input buffer overfull
    txrdy = 1           // transmission done
;

static int  rbufno = 0; // current rbuf number
static char rbuf[2][UARTBUFSZ], tbuf[UARTBUFSZ]; // receive & transmit buffers
static char *recvdata = NULL;

static char trbuf[UARTBUFSZ]; // auxiliary buffer for data transmission
int trbufidx = 0;
int put2trbuf(char c){
    if(trbufidx > UARTBUFSZ - 1) return 1;
    trbuf[trbufidx++] = c;
    return 0;
}
// write zero-terminated string
int write2trbuf(const char *str){
    while(trbufidx < UARTBUFSZ - 1 && *str){
        trbuf[trbufidx++] = *str++;
    }
    if(*str) return 1; // buffer overfull
    trbuf[trbufidx] = 0;
    return 0; // all OK
}
char *gettrbuf(){
    if(trbufidx > UARTBUFSZ - 1) trbufidx = UARTBUFSZ - 1;
    trbuf[trbufidx] = 0;
    return trbuf;
}


void USART1_config(){
    /* Enable the peripheral clock of GPIOA */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* GPIO configuration for USART1 signals */
    /* (1) Select AF mode (10) on PA9 and PA10 */
    /* (2) AF1 for USART1 signals */
    GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9|GPIO_MODER_MODER10))\
                | (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1); /* (1) */
    GPIOA->AFR[1] = (GPIOA->AFR[1] &~ (GPIO_AFRH_AFRH1 | GPIO_AFRH_AFRH2))\
                | (1 << (1 * 4)) | (1 << (2 * 4)); /* (2) */
    // Tx (PA9) in OD mode
    GPIOA->OTYPER |= 1 << 9;
    #ifdef EBUG
    GPIOA->PUPDR = (GPIOA->PUPDR & ~GPIO_PUPDR_PUPDR9) | GPIO_PUPDR_PUPDR9_0; // set pullup for Tx
    #endif
    /* Enable the peripheral clock USART1 */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    /* Configure USART1 */
    /* (1) oversampling by 16, 115200 baud */
    /* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */
    USART1->BRR = 480000 / 1152; /* (1) */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; /* (2) */
    /* polling idle frame Transmission */
    while(!(USART1->ISR & USART_ISR_TC)){}
    USART1->ICR |= USART_ICR_TCCF; /* clear TC flag */
    USART1->CR1 |= USART_CR1_RXNEIE; /* enable TC, TXE & RXNE interrupt */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel2->CPAR = (uint32_t) &(USART1->TDR); // periph
    DMA1_Channel2->CMAR = (uint32_t) tbuf; // mem
    DMA1_Channel2->CCR |= DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE; // 8bit, mem++, mem->per, transcompl irq
    USART1->CR3 = USART_CR3_DMAT;
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    /* Configure IT */
    /* (3) Set priority for USART1_IRQn */
    /* (4) Enable USART1_IRQn */
    NVIC_SetPriority(USART1_IRQn, 0); /* (3) */
    NVIC_EnableIRQ(USART1_IRQn); /* (4) */
}

void usart1_isr(){
    #ifdef CHECK_TMOUT
    static uint32_t tmout = 0;
    #endif
    if(USART1->ISR & USART_ISR_RXNE){ // RX not emty - receive next char
        #ifdef CHECK_TMOUT
        if(tmout && Tms >= tmout){ // set overflow flag
            bufovr = 1;
            datalen[rbufno] = 0;
        }
        tmout = Tms + TIMEOUT_MS;
        if(!tmout) tmout = 1; // prevent 0
        #endif
        // read RDR clears flag
        uint8_t rb = USART1->RDR;
        if(datalen[rbufno] < UARTBUFSZ){ // put next char into buf
            rbuf[rbufno][datalen[rbufno]++] = rb;
            if(rb == '\n'){ // got newline - line ready
                linerdy = 1;
                dlen = datalen[rbufno];
                recvdata = rbuf[rbufno];
                recvdata[dlen-1] = 0; // change '\n' to trailing zero
                // prepare other buffer
                rbufno = !rbufno;
                datalen[rbufno] = 0;
                #ifdef CHECK_TMOUT
                // clear timeout at line end
                tmout = 0;
                #endif
            }
        }else{ // buffer overrun
            bufovr = 1;
            datalen[rbufno] = 0;
            #ifdef CHECK_TMOUT
            tmout = 0;
            #endif
        }
    }
}

void dma1_channel2_3_isr(){
    if(DMA1->ISR & DMA_ISR_TCIF2){ // Tx
        DMA1->IFCR |= DMA_IFCR_CTCIF2; // clear TC flag
        txrdy = 1;
    }
}

/**
 * return length of received data (without trailing zero
 */
int usart1_getline(char **line){
    if(!linerdy) return 0;
    if(bufovr){
        bufovr = 0;
        linerdy = 0;
        return 0;
    }
    *line = recvdata;
    linerdy = 0;
    return dlen;
}

/*
// send bu UART zero-terminated string `str` with length `len` (with substitution of trailing zero by '\n')
TXstatus usart1_send(char *str){
    if(!txrdy) return LINE_BUSY;
    int len = 0;
    while(len < UARTBUFSZ && str[len]) ++len;
    if(len > UARTBUFSZ-1) return STR_TOO_LONG;
    str[len++] = '\n';
    txrdy = 0;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    memcpy(tbuf, str, len);
    DMA1_Channel2->CNDTR = len;
    DMA1_Channel2->CCR |= DMA_CCR_EN; // start transmission
    return ALL_OK;
}*/

TXstatus usart1_send(char *str){
    if(!txrdy) return LINE_BUSY;
    int i;
    for(i = 0; i < UARTBUFSZ; ++i){
        char c = *str++;
        if(c == 0){ c = '\n'; i = UARTBUFSZ;}
        USART1->TDR = c;
        while(!(USART1->ISR & USART_ISR_TXE));
    }
    txrdy = 1;
    return ALL_OK;
}


/**
 * Fill trbuf with integer value
 * @param N - integer value
 * @return 1 if buffer overflow; oterwise return 0
 */
int put_int(int32_t N){
    if(N < 0){
        if(put2trbuf('-')) return 1;
        N = -N;
    }
    return put_uint((uint32_t) N);
}

int put_uint(uint32_t N){
    char buf[10];
    int L = 0;
    if(N){
        while(N){
            buf[L++] = N % 10 + '0';
            N /= 10;
        }
        while(L--) if(put2trbuf(buf[L])) return 1;
    }else if(put2trbuf('0')) return 1;
    return 0;
}
