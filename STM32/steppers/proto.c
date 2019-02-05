/*
 *                                                                                                  geany_encoding=koi8-r
 * proto.c
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
#include "adc.h"
#include "flash.h"
#include "proto.h"
#include "steppers.h"
#include "stm32f0.h"
#include "string.h"
#include "usart.h"

static const char *eodata = "DATAEND";
static const char *badcmd = "BADCMD";
static const char *allok = "ALLOK";
static const char *err = "ERR";

#define EODATA  (eodata)
#define BADCMD  (badcmd)
#define ALLOK   (allok)
#define ERR     (err)

static const char *getnum(const char *buf, int32_t *N);
static const char *get_something(const char *str);
static const char *set_something(const char *str);
static const char *motor_cmd(const char *str);

static const char *get_status(void);
static const char *get_conf(void);
static const char *get_raw_adc(void);
static const char *get_ADCval(const char *str);
static const char *get_temper(void);

static const char *setDenEn(uint8_t De, const char *str);
static const char *setDevId(const char *str);
static const char *setESWthres(const char *str);
static const char *setUSARTspd(const char *str);
static const char *setUSTEPS(const char *str);
static const char *setACCDEC(const char *str);
static const char *setmotvals(char v, const char *str);
static const char *setMotSpeed(int cur, const char *str);

#define omitwsp(str) do{register char nxt; while((nxt = *str)){if(nxt != ' ' && nxt != '\t') break; else ++str;}}while(0)

/**
 * get input buffer `cmdbuf`, parse it and change system state
 * @return message to send
 */
const char* process_command(const char *cmdbuf){
    int32_t num;
    const char *str;
    char c;
    #ifdef EBUG
    usart1_send_blocking(cmdbuf);
    #endif
    str = getnum(cmdbuf, &num);
    if(!str) return NULL; // bad format
    if(num != the_conf.devID && num != -1) return NULL; // other target
    // OK, the command is for this device
    while((c = *str)){if(c != ' ' && c != '\t') break; else ++str;}
    if(!c){ // simple ping
        return "ALIVE";
    }
    switch (*str++){
        case 'G': // get something
            return get_something(str);
        break;
        case 'M': // motors' management
            return motor_cmd(str);
        break;
        case 'R':
            NVIC_SystemReset();
        break;
        case 'S': // set something
            return set_something(str);
        break;
        case 'W': // write flash
            if(store_userconf()) return ERR;
            else return ALLOK;
        break;
    }
    return BADCMD; // badcmd
}

// read `buf` and get first integer `N` in it
// @return pointer to first non-number if all OK or NULL if first symbol isn't a space or number
static const char *getnum(const char *buf, int32_t *N){
    char c;
    int positive = -1;
    int32_t val = 0;
    while((c = *buf++)){
        if(c == '\t' || c == ' '){
            if(positive < 0) continue; // beginning spaces
            else break; // spaces after number
        }
        if(c == '-'){
            if(positive < 0){
                positive = 0;
                continue;
            }else break; // there already was `-` or number
        }
        if(c < '0' || c > '9') break;
        if(positive < 0) positive = 1;
        val = val * 10 + (int32_t)(c - '0');
    }
    if(positive != -1){
        if(positive == 0){
            if(val == 0) return NULL; // single '-'
            val = -val;
        }
        *N = val;
    }else return NULL;
    return buf-1;
}

// get conf (uint16_t) number
// @return 0 if all OK
static int getu16(const char *buf, uint16_t *N){
    int32_t N32;
    if(!getnum(buf, &N32)) return 1;
    if(N32 > 0xffff || N32 < 0) return 1;
    *N = (uint16_t) N32;
    return 0;
}

static const char *get_something(const char *str){
    switch(*str++){
        case 'A': // get ADC value: voltage or current
            return get_ADCval(str);
        break;
        case 'C': // get current configuration values
            return get_conf();
        break;
        case 'R': // get raw ADC values
            return get_raw_adc();
        break;
        case 'S': // get motors' status
            return get_status();
        break;
        case 'T': // get MCU temperature
            return get_temper();
        break;
    }
    return BADCMD;
}

static const char *get_status(){
    char str[3] = {0, '=', 0};
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        write2trbuf("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        write2trbuf("SOFTRESET=1\n");
    }
    RCC->CSR = RCC_CSR_RMVF; // clear reset flags
    for(int8_t i = 0; i < 2; ++i){
        write2trbuf("MOTOR"); str[0] = '0' + i;
        write2trbuf(str);
        stp_state stt = stp_getstate(i);
        if(STP_SLEEP != stt){
            char *s;
            switch(stt){
                case STP_ACCEL:
                    s = "ACCEL";
                break;
                case STP_DECEL:
                    s = "DECEL";
                break;
                case STP_MOVE:
                    s = "MOVE";
                break;
                case STP_MOVE0:
                    s = "MOVETO0";
                break;
                case STP_MOVE1:
                    s = "MOVETO1";
                break;
                case STP_MVSLOW:
                    s = "MVSLOW";
                break;
                case STP_STOP:
                    s = "STOP";
                break;
                case STP_STOPZERO:
                    s = "STOPZERO";
                break;
                default:
                    s = "UNKNOWN";
            }
            write2trbuf(s);
            write2trbuf("\nSTEPSLEFT");
            write2trbuf(str);
            put_uint(stp_stepsleft(i));
        }else write2trbuf("SLEEP");
        write2trbuf("\nPOS");
        write2trbuf(str);
        put_int(stp_position(i));
        SENDBUF();
        for(int8_t j = 0; j < 2; ++j){
            write2trbuf("ESW"); put2trbuf('0' + i);
            put2trbuf('0' + j); put2trbuf('=');
            ESW_status stat = eswStatus(i, j);
            const char *etxt = "ERR";
            switch(stat){
                case ESW_RELEASED:
                    etxt = "RLSD";
                break;
                case ESW_BUTTON:
                    etxt = "BTN";
                break;
                case ESW_HALL:
                    etxt = "HALL";
                break;
                default:
                break;
            }
            write2trbuf(etxt);
            SENDBUF();
        }
    }
    return NULL;
}

typedef struct{
    const char *fieldname;
    const uint16_t *ptr;
} user_conf_descr;

typedef struct{
    const char *fieldname;
    const uint8_t *ptr;
} user_conf_descr8;

static const user_conf_descr descrarr[] = {
    {"CONFSZ",      &the_conf.userconf_sz},
    {"DEVID",       &the_conf.devID},
    {"V12NUM",      &the_conf.v12numerator},
    {"V12DEN",      &the_conf.v12denominator},
    {"I12NUM",      &the_conf.i12numerator},
    {"I12DEN",      &the_conf.i12denominator},
    {"V33NUM",      &the_conf.v33numerator},
    {"V33DEN",      &the_conf.v33denominator},
    {"ESWTHR",      &the_conf.ESW_thres},
    {"MOT0SPD",     &the_conf.motspd[0]},
    {"MOT1SPD",     &the_conf.motspd[1]},
    {"MAXSTEPS0",   &the_conf.maxsteps[0]},
    {"MAXSTEPS1",   &the_conf.maxsteps[1]},
    {NULL, NULL}
};

static const user_conf_descr8 descrarr8[] = {
    {"INTPULLUP",   &the_conf.intpullup},
    {"REVERSE0",    &the_conf.reverse[0]},
    {"REVERSE1",    &the_conf.reverse[1]},
    {"USTEPS",      &the_conf.usteps},
    {"ACCDECSTEPS", &the_conf.accdecsteps},
    {NULL, NULL}
};

static const char *get_conf(){
    const user_conf_descr *curdesc = descrarr;
    do{
        write2trbuf(curdesc->fieldname);
        put2trbuf('=');
        put_uint((uint32_t) *curdesc->ptr);
        SENDBUF();
    }while((++curdesc)->fieldname);
    const user_conf_descr8 *curdesc8 = descrarr8;
    write2trbuf("USARTSPD=");
    put_uint(the_conf.usartspd);
    SENDBUF();
    do{
        write2trbuf(curdesc8->fieldname);
        put2trbuf('=');
        put_uint((uint32_t) *curdesc8->ptr);
        SENDBUF();
    }while((++curdesc8)->fieldname);
    return EODATA;
}

static const char *get_raw_adc(){
    for(int8_t i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
        write2trbuf("ADC[");
        put2trbuf('0' + i);
        write2trbuf("]=");
        put_uint((uint32_t) getADCval(i));
        SENDBUF();
    }
    return EODATA;
}

static const char *get_ADCval(const char *str){
    uint32_t v;
    switch(*str){
        case 'D': // vdd
            write2trbuf("VDD=");
            v = getVdd();
        break;
        case 'I': // motors' current
            write2trbuf("IMOT=");
            v = getImot();
        break;
        case 'M': // vmot
            write2trbuf("VMOT=");
            v = getVmot();
        break;
        default:
            return BADCMD;
    }
    put_uint(v);
    SENDBUF();
    return NULL;
}

static const char *get_temper(){
    int32_t t = getTemp();
    write2trbuf("TEMP=");
    put_int(t);
    SENDBUF();
    return NULL;
}

static const char *set_something(const char *str){
    switch(*str++){
        case 'A': // set accdecsteps
            return setACCDEC(str);
        break;
        case 'C': // set current speed
            return setMotSpeed(1, str);
        break;
        case 'D': // set denominator
            return setDenEn(1, str);
        break;
        case 'E': // set numerator
            return setDenEn(0, str);
        break;
        case 'I': // set device ID
            return setDevId(str);
        break;
        case 'M': // set maxsteps
            return setmotvals('M', str);
        break;
        case 'P': // set pullup
            omitwsp(str);
            if(*str == '0') the_conf.intpullup = 0;
            else the_conf.intpullup = 1;
            return ALLOK;
        break;
        case 'R': // set reverse
            return setmotvals('R', str);
        break;
        case 'S': // set speed
            return setMotSpeed(0, str);
        break;
        case 'T': // set endsw threshold
            return setESWthres(str);
        break;
        case 'U': // set USART speed
            return setUSARTspd(str);
        break;
        case 'u': // set usteps
            return setUSTEPS(str);
        break;
    }
    return BADCMD;
}

/**
 * set denominator/numerator
 * @param De == 1 for denominator, == 0 for numerator
 * @param str - rest of string
 */
static const char *setDenEn(uint8_t De, const char *str){
    uint16_t *targ = NULL;
    switch(*str++){
        case 'D':
            targ = De ? &the_conf.v33denominator : &the_conf.v33numerator;
        break;
        case 'I':
            targ = De ? &the_conf.i12denominator : &the_conf.i12numerator;
        break;
        case 'M':
            targ = De ? &the_conf.v12denominator : &the_conf.v12numerator;
        break;
        default:
            return BADCMD;
    }
    omitwsp(str);
    if(getu16(str, targ)) return BADCMD;
    return ALLOK;
}

static const char *setDevId(const char *str){
    omitwsp(str);
    if(getu16(str, &the_conf.devID)) return BADCMD;
    return ALLOK;
}

static const char *setESWthres(const char *str){
    omitwsp(str);
    if(getu16(str, &the_conf.ESW_thres)) return BADCMD;
    return ALLOK;
}

static const char *setUSARTspd(const char *str){
    omitwsp(str);
    int32_t N32;
    if(!getnum(str, &N32)) return BADCMD;
    the_conf.usartspd = (uint32_t) N32;
    return ALLOK;
}

// if cur == 1 set current speed else set global motspd
static const char *setMotSpeed(int cur, const char *str){
    omitwsp(str);
    uint8_t Num = (uint8_t)(*str++ - '0');
    if(Num > 1) return ERR;
    int32_t spd;
    omitwsp(str);
    if(!getnum(str, &spd)) return BADCMD;
    if(spd < 2 || spd > (0xffff/LOWEST_SPEED_DIV)) return ERR;
    if(cur){ // change current speed
        stp_chARR(Num, spd);
    }else{
        the_conf.motspd[Num] = (uint16_t)spd;
        stp_chspd();
    }
    return ALLOK;
}

// set other motor values
static const char *setmotvals(char v, const char *str){
    omitwsp(str);
    uint8_t Num = (uint8_t)(*str++ - '0');
    if(Num > 1) return ERR;
    omitwsp(str);
    int32_t val;
    if(!getnum(str, &val)) return BADCMD;
    if(val < 0 || val > 0xffff) return ERR;
    switch(v){
        case 'M': // maxsteps
            if(val == 0) return ERR;
            the_conf.maxsteps[Num] = (uint16_t)val;
        break;
        case 'R': // reverse
            if(val && val != 1) return ERR;
            the_conf.reverse[Num] = (uint8_t)val;
        break;
        default: return ERR;
    }
    return ALLOK;
}

// process motor command: start/stop
static const char *motor_cmd(const char *str){
    omitwsp(str);
    uint8_t Num = (uint8_t)(*str++ - '0');
    int32_t steps;
    stp_status st;
    if(Num > 1) return ERR;
    omitwsp(str);
    switch(*str++){
        case 'M':
            omitwsp(str);
            if(!getnum(str, &steps)) return "BadSteps";
            st = stp_move(Num, steps);
            switch(st){
                case STPS_ACTIVE:
                    return "IsMoving";
                break;
                case STPS_ONESW:
                    return "OnEndSwitch";
                break;
                case STPS_ZEROMOVE:
                    return "ZeroMove";
                break;
                case STPS_TOOBIG:
                    return "TooBigNumber";
                break;
                default:
                    return ALLOK;
            }
        break;
        case 'S':
            stp_stop(Num);
            return ALLOK;
        break;
    }
    return ERR;
}

// set value of usteps
static const char *setUSTEPS(const char *str){
    omitwsp(str);
    uint16_t x;
    if(getu16(str, &x)) return BADCMD;
    switch(x){
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
            the_conf.usteps = (uint8_t)x;
        break;
        default:
            return ERR;
    }
    return ALLOK;
}

// set value of accdecsteps
static const char *setACCDEC(const char *str){
    omitwsp(str);
    uint16_t x;
    if(getu16(str, &x)) return BADCMD;
    if(x < ACCDECSTEPS_MIN || x > 255) return ERR;
    the_conf.accdecsteps = (uint8_t)x;
    stp_chspd();
    return ALLOK;
}
