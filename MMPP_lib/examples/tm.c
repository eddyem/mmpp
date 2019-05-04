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

/******************************************************************************
 *                         Motors testing tool                                *
 ******************************************************************************/

#include "tmcmdlnopts.h"
#include <signal.h>
#include <stdio.h>
#include <libmmpp.h>
#include <usefull_macros.h>

// All return states of main():
enum{
    RET_ALLOK = 0,
    RET_NOTFOUND,       // none of MCU found or didn't found seeking MCU
    RET_ONLYONE,        // only one MCU found
    RET_COMMERR,        // communication error
    RET_CANTINIT,       // can't init motors: error during initiation or some of motors are moving
    RET_WAITERR,        // error occured during waiting procedure
    RET_ERROR = 9,      // uncoverable error - from libsnippets
    RET_HELPCALL = 255  // user call help (or give wrong parameter[s]) - from libsnippets
};
static glob_pars *G;

/**
 * We REDEFINE the default WEAK function of signal processing
 */
void __attribute__((noreturn)) signals(int sig){
    if(sig){
        signal(sig, SIG_IGN);
        DBG("Get signal %d, quit.\n", sig);
    }
    if(G->pidfile) // remove unnesessary PID file
        unlink(G->pidfile);
    restore_console();
    mmpp_close();
    exit(sig);
}

static double convangle(double val){
    int X = (int)(val / 360.);
    val -= 360. * (double)X;
    return val;
}

static void parsestatus(ttysend_status st){
    if(quiet) return;
    switch(st){
        case SEND_ERR:
           red("communication error");
        break;
        case SEND_ALLOK:
            green("all OK");
        break;
        case SEND_ACTIVE:
            red("motor is still moving");
        break;
        case SEND_TOOBIG:
            red("the steps amount is too large");
        break;
        case SEND_ZEROMOVE:
            green("already at position");
        break;
        case SEND_ESWITCH:
            red("on end-switch and can't move further");
        break;
        case SEND_NEEDINIT:
            red("motors aren't initialised");
        break;
        case SEND_NEGATMOVE:
            red("try to move into negative position");
        break;
        case SEND_OTHER:
        default:
            red("other error (wrong motor/MCU number?)");
    }
    printf("\n");
}

static void pollstatus(){
    motor_state S;
    bool mvng[3] = {false, true, true}, starting = true;
    int curpos[4];
    double adc[6];
    while(mvng[1] || mvng[2]){
        for(int Nmcu = 1; Nmcu < 3; ++Nmcu){
            if(!mot_getstatus(Nmcu, &S)){
                mvng[Nmcu] = false;
                continue;
            }
            if(S.state[0] == STP_SLEEP && S.state[1] == STP_SLEEP)
                mvng[Nmcu] = false;
            curpos[2*(Nmcu-1)+0] = S.curpos[0];
            curpos[2*(Nmcu-1)+1] = S.curpos[1];
            if(G->getADC){
                ADC_state s;
                for(int Nmcu = 1; Nmcu < 3; ++Nmcu){
                    if(get_ADC(Nmcu, &s)){
                        adc[3*(Nmcu-1)+0] = s.Vdd;
                        adc[3*(Nmcu-1)+1] = s.Imot;
                        adc[3*(Nmcu-1)+2] = s.Vmot;
                    }
                }
            }
        }
        if(!mvng[1] && !mvng[2]){
            if(starting){
                starting = false;
                continue;
            }
        }else starting = false;
        if(!quiet){
            printf("1: %6d, %6d", curpos[0],curpos[1]);
            if(G->getADC){
                printf(", VDD=%-5.1f Imot=%-5.1f Vmot=%-5.1f", adc[0], adc[1], adc[2]);
            }
            printf("   2: %6d, %6d", curpos[2],curpos[3]);
            if(G->getADC){
                printf(", VDD=%-5.1f Imot=%-5.1f Vmot=%-5.1f", adc[3], adc[4], adc[5]);
            }
            printf("           \r");
        }
    }
    printf("\n\n");
}

int main(int argc, char **argv){
    initial_setup();
    signal(SIGTERM, signals);   // kill (-15)
    signal(SIGINT, signals);    // ctrl+C
    signal(SIGQUIT, SIG_IGN);   // ctrl+\   .
    signal(SIGTSTP, SIG_IGN);   // ctrl+Z
    setbuf(stdout, NULL);
    G = parse_args(argc, argv);
    check4running(NULL, G->pidfile);
    DBG("Try to open serial %s", G->comdev);
    if(mmpp_tryopen(G->comdev, G->speed)){
        ERRX(_("Can't open %s with speed %d. Exit."), G->comdev, G->speed);
    }
    if(mot_handshake()){
        WARNX(_("Controllers not found"));
        signals(RET_NOTFOUND);
    }
    for(int i = 1; i < 3; ++i){
        if(get_alive(i)){
            if(!quiet) green("MCU #%d found\n", i);
        }else WARNX(_("MCU #%d not found"), i);
    }
    if(G->stopall){
        int r = stop_all();
        if(r) WARNX(_("Error for %d motors of 4"), r);
        else MSG("Successfully send command to stop all", NULL);
    }
    if(G->gettemp){
        double t1, t2;
        if(get_temp(&t1, &t2)){
            MSG("Got MCU temp:", NULL);
            if(t1 > -300.) printf("\tTMCU1=%gdegrC\n", t1);
            if(t2 > -300.) printf("\tTMCU2=%gdegrC\n", t2);
        }else WARNX(_("Can't get MCU temp"));
    }
    if(G->getstatus){
        for(int N = 1; N < 3; ++N){
            motor_state s;
            if(mot_getstatus(N, &s)){
                if(get_rst(N, true))
                    WARNX(_("Controller #%d was in reset state"), N);
                if(!quiet) green("MCU#%d state:\n", N);
                for(int i = 0; i < 2; ++i){
                    printf("\tSTATE[%d]=%d\n",i, s.state[i]);
                    printf("\tSTEPSLEFT[%d]=%d\n", i, s.stepsleft[i]);
                    printf("\tCURPOS[%d]=%d\n", i, s.curpos[i]);
                    for(int j = 0; j < 2; ++j)
                        printf("\tESW_STATE[%d][%d]=%d\n", i, j, s.ESW_status[i][j]);
                }
            }
        }
    }
    if(G->sendraw){
        char **raw = G->sendraw;
        do{
            MSG(_("Send raw string"), *raw);
            char *got = tty_sendraw(*raw);
            if(got){
                MSG(_("Receive"), got);
                if(quiet) printf("%s", got);
            }else WARNX(_("Nothing received"));
        }while(*(++raw));
    }
    if(G->getADC){
        MSG(_("Get ADC values"), NULL);
        ADC_state s;
        for(int i = 1; i < 3; ++i){
            if(get_ADC(i, &s)){
                printf("VDD%d=%g\n", i, s.Vdd);
                printf("IMOT%d=%g\n", i, s.Imot);
                printf("VMOT%d=%g\n", i, s.Vmot);
            }else WARNX(_("Can't get ADC values for MCU#%d"), i);
        }
    }
    if(G->rot1angle > -999. || G->rot2angle > -999. || G->l1steps != INT_MAX || G->l2steps != INT_MAX){
        // all other commands are tied with moving, so check if motors are inited
        MSG("Init motors", NULL);
        // move all uninitialized motors to their zero position
        int ini = init_motors(); // BLOCKING call!!!
        // you can use non-blocking initialisation by proper rewriting of `init_motors`
        if(ini){
            WARNX(_("Can't init motors: %d\n"), ini);
            signals(RET_CANTINIT);
        }else green("Motors are ready!\n");
        ttysend_status st;
        if(G->rot1angle > -999.){
            double angle = convangle(G->rot1angle);
            int steps = (int)((STEPSREV1/360.) * angle);
            MSG("Try to rotate polaroid ...", NULL);
            st = movemotor(1, 1, steps, G->absmove);
            parsestatus(st);
        }
        if(G->rot2angle > -999.){
            double angle = convangle(G->rot2angle);
            int steps = (int)((STEPSREV2/360.) * angle);
            MSG("Try to rotate waveplate ...", NULL);
            st = movemotor(2, 1, steps, G->absmove);
            parsestatus(st);
        }
        if(G->l1steps != INT_MAX){
            MSG("Try to move polaroid stage ...", NULL);
            st = movemotor(1, 0, G->l1steps, G->absmove);
            parsestatus(st);
        }
        if(G->l2steps != INT_MAX){
            MSG("Try to move waveplate stage ...", NULL);
            st = movemotor(2, 0, G->l2steps, G->absmove);
            parsestatus(st);
        }
    }
    pollstatus();
    if(G->reset){
        int **N = G->reset;
        while(*N){
            if(!quiet) green("Reset controller #%d\n", **N);
            reset_MCU(**N);
            ++N;
        }
    }
    signals(0);
}
