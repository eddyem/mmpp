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
#include "stm32f0.h"
#include "proto.h"
#include "adc.h"
#include "flash.h"
#include "string.h"
#include "usart.h"
#include "steppers.h"

static const char *eodata = "DATAEND";
static const char *badcmd = "BADCMD";
static const char *allok = "ALL OK";
static const char *err = "ERR";

#define EODATA  ((char*)eodata)
#define BADCMD  ((char*)badcmd)
#define ALLOK   ((char*)allok)
#define ERR     ((char*)err)

static char *getnum(char *buf, int32_t *N);
static char *get_something(char *str);
static char *set_something(char *str);
static char *motor_cmd(char *str);

static char *get_status();
static char *get_conf();
static char *get_raw_adc();
static char *get_ADCval(char *str);
static char *get_temper();

static char *setDenEn(uint8_t De, char *str);
static char *setDevId(char *str);
static char *setESWthres(char *str);
static char *setUSARTspd(char *str);
static char *setmotvals(char v, char *str);
static char *setMotSpeed(int cur, char *str);

#define omitwsp(str) do{register char nxt; while((nxt = *str)){if(nxt != ' ' && nxt != '\t') break; else ++str;}}while(0)

/**
 * get input buffer `cmdbuf`, parse it and change system state
 * @return message to send
 */
char* process_command(char *cmdbuf){
    int32_t num;
    char *str, c;
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
static char *getnum(char *buf, int32_t *N){
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
static int getu16(char *buf, uint16_t *N){
    int32_t N32;
    if(!getnum(buf, &N32)) return 1;
    if(N32 > 0xffff || N32 < 0) return 1;
    *N = (uint16_t) N32;
    return 0;
}

static char *get_something(char *str){
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

static char *get_status(){
    int i, j;
    char str[3] = {0, '=', 0};
    if(RCC->CSR & RCC_CSR_IWDGRSTF){ // watchdog reset occured
        write2trbuf("WDGRESET=1\n");
    }
    if(RCC->CSR & RCC_CSR_SFTRSTF){ // software reset occured
        write2trbuf("SOFTRESET=1\n");
    }
    RCC->CSR = RCC_CSR_RMVF; // clear reset flags
    for(i = 0; i < 2; ++i){
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
        for(j = 0; j < 2; ++j){
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

static const user_conf_descr descrarr[] = {
    {"CONFSZ", &the_conf.userconf_sz},
    {"DEVID",  &the_conf.devID},
    {"V12NUM", &the_conf.v12numerator},
    {"V12DEN", &the_conf.v12denominator},
    {"I12NUM", &the_conf.i12numerator},
    {"I12DEN", &the_conf.i12denominator},
    {"V33NUM", &the_conf.v33numerator},
    {"V33DEN", &the_conf.v33denominator},
    {"ESWTHR", &the_conf.ESW_thres},
    {"MOT0SPD",&the_conf.motspd[0]},
    {"MOT1SPD",&the_conf.motspd[1]},
    {"MAXSTEPS0",&the_conf.maxsteps[0]},
    {"MAXSTEPS1",&the_conf.maxsteps[1]},
    {NULL, NULL}
};

static char *get_conf(){
    const user_conf_descr *curdesc = descrarr;
    do{
        write2trbuf(curdesc->fieldname);
        put2trbuf('=');
        put_uint((uint32_t) *curdesc->ptr);
        SENDBUF();
    }while((++curdesc)->fieldname);
    write2trbuf("INTPULLUP=");
    put2trbuf(the_conf.intpullup ? '1' : '0');
    write2trbuf("\nUSARTSPD=");
    put_uint(the_conf.usartspd);
    SENDBUF();
    write2trbuf("REVERSE0=");
    put_uint(the_conf.reverse[0]);
    write2trbuf("\nREVERSE1=");
    put_uint(the_conf.reverse[1]);
    SENDBUF();
    return EODATA;
}

static char *get_raw_adc(){
    int i;
    for(i = 0; i < NUMBER_OF_ADC_CHANNELS; ++i){
        write2trbuf("ADC[");
        put2trbuf('0' + i);
        write2trbuf("]=");
        put_uint((uint32_t) ADC_array[i]);
        SENDBUF();
    }
    return EODATA;
}

static char *get_ADCval(char *str){
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

static char *get_temper(){
    int32_t t = getTemp();
    write2trbuf("TEMP=");
    put_int(t);
    SENDBUF();
    return NULL;
}

static char *set_something(char *str){
    switch(*str++){
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
            setmotvals('M', str);
        break;
        case 'P': // set pullup
            omitwsp(str);
            if(*str == '0') the_conf.intpullup = 0;
            else the_conf.intpullup = 1;
        break;
        case 'R': // set reverse
            setmotvals('R', str);
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
    }
    return BADCMD;
}

/**
 * set denominator/numerator
 * @param De == 1 for denominator, == 0 for numerator
 * @param str - rest of string
 */
static char *setDenEn(uint8_t De, char *str){
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

static char *setDevId(char *str){
    omitwsp(str);
    if(getu16(str, &the_conf.devID)) return BADCMD;
    return ALLOK;
}

static char *setESWthres(char *str){
    omitwsp(str);
    if(getu16(str, &the_conf.ESW_thres)) return BADCMD;
    return ALLOK;
}

static char *setUSARTspd(char *str){
    omitwsp(str);
    int32_t N32;
    if(!getnum(str, &N32)) return BADCMD;
    the_conf.usartspd = (uint32_t) N32;
    return ALLOK;
}

// if cur == 1 set current speed else set global motspd
static char *setMotSpeed(int cur, char *str){
    omitwsp(str);
    uint8_t Num = *str++ - '0';
    if(Num > 1) return ERR;
    int32_t spd;
    omitwsp(str);
    if(!getnum(str, &spd)) return ERR;
    if(spd < 2 || spd > 6553) return "BadSpd";
    if(cur){ // change current speed
        stp_chARR(Num, spd);
    }else{
        the_conf.motspd[Num] = spd;
        stp_chspd();
    }
    return ALLOK;
}

// set other motor values
static char *setmotvals(char v, char *str){
    omitwsp(str);
    uint8_t Num = *str++ - '0';
    if(Num > 1) return ERR;
    omitwsp(str);
    int32_t val;
    if(!getnum(str, &val)) return ERR;
    if(val < 0 || val > 0xffff) return "BadUINT16";
    switch(v){
        case 'M': // maxsteps
            if(val == 0) return ERR;
            the_conf.maxsteps[Num] = val;
        break;
        case 'R': // reverse
            if(val && val != 1) return ERR;
            the_conf.reverse[Num] = val;
        break;
        default: return ERR;
    }
    return ALLOK;
}

// process motor command: start/stop
static char *motor_cmd(char *str){
    omitwsp(str);
    uint8_t Num = *str++ - '0';
    int32_t steps;
    stp_status st;
    if(Num > 1) return "Num>1";
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
