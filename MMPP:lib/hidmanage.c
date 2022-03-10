/*
 * hidmanage.c - manage HID devices
 *
 * Copyright 2016 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "common.h"
#include "hsfw.h"
#include "libmmpp.h"
#include <libudev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <unistd.h>

static bool check_and_clear_err(int fd);

/**
 * @brief writereg - write buffer to register & check answer
 * @param fd  - wheel fd
 * @param buf - data to write
 * @param l   - lenght of `buf`
 * @return  0 if all OK
 */
static int writereg(int fd, uint8_t *buf, int l){
    uint8_t reg = buf[0];
    if(ioctl(fd, HIDIOCSFEATURE(l), buf) < 0 || buf[0] != reg){
        DBG("can't write register %d", reg);
        return 1;
    }
    return 0;
}

/**
 * @brief readreg - read register to buf & check answer
 * @param fd  - wheel fd
 * @param buf - buffer for reading
 * @param reg - register to read
 * @param l   - data length
 * @return 0 if all OK
 */
static int readreg(int fd, uint8_t *buf, int reg, int l){
    memset(buf, 0, l);
    buf[0] = reg;
    if(ioctl(fd, HIDIOCGFEATURE(l), buf) < 0 || buf[0] != reg){
        DBG("can't read register %d", reg);
        return 1;
    }
    return 0;
}

/**
 * poll status register until moving stops
 * @param fd  - turret file descriptor
 * @param msg - ==1 to show message
 * @return current position or error code
 */
static wheel_status poll_wheelstatus(int fd){
    uint8_t buf[REG_STATUS_LEN];
    int i, stat = 1;
    for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
        stat = readreg(fd, buf, REG_STATUS, REG_STATUS_LEN);
        if(!stat){
            if(buf[2] == 0xff || buf[3] == 0xff) stat = 1;
        }
        if(buf[5]){
            return WHEEL_ERROR;
        }
    }
    if(i == HANDSHAKE_TRIES) return WHEEL_MOVING;
    return buf[4];
}

// library function for position polling
wheel_status wheel_getpos(wheel_descr *w){
    return poll_wheelstatus(w->fd);
}

/**
 * @brief go_home - blocking go to home position
 * @param fd - wheel file descriptor
 * @return true if all OK
 */
static bool go_home(int fd){
    poll_wheelstatus(fd); // wait for last moving
    uint8_t buf[REG_HOME_LEN];
    int i, stat = 1;
    for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
        memset(buf, 0, REG_HOME_LEN);
        buf[0] = REG_HOME;
        stat = writereg(fd, buf, REG_HOME_LEN);
        if(stat){usleep(100000); continue;}
        if((stat = readreg(fd, buf, REG_HOME, REG_HOME_LEN))) continue;
        if(buf[1] != 0xff){
            stat = 1; continue;
        }else{
            readreg(fd, buf, REG_HOME, REG_HOME_LEN);
            break;
        }
    }
    if(i == HANDSHAKE_TRIES) return false;
    // now poll REG_STATUS
    while(WHEEL_MOVING == poll_wheelstatus(fd)){DBG("still moving");};
    check_and_clear_err(fd);
    return true;
}

// the same as above, but non-blocking and library function
bool wheel_home(wheel_descr *w){
    int fd = w->fd;
    poll_wheelstatus(fd); // wait for last moving
    uint8_t buf[REG_HOME_LEN];
    int i, stat = 1;
    for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
        memset(buf, 0, REG_HOME_LEN);
        buf[0] = REG_HOME;
        stat = writereg(fd, buf, REG_HOME_LEN);
        if(stat){usleep(100000); continue;}
        if((stat = readreg(fd, buf, REG_HOME, REG_HOME_LEN))) continue;
        if(buf[1] != 0xff){
            stat = 1; continue;
        }else{
            readreg(fd, buf, REG_HOME, REG_HOME_LEN);
            break;
        }
    }
    if(i == HANDSHAKE_TRIES) return false;
    return true;
}

/**
 * @brief check_and_clear_err - check error state and clear it if need
 * @param fd - opened device file descriptor
 * @return true if all OK
 */
static bool check_and_clear_err(int fd){
    int i, stat = 1;
    uint8_t buf[REG_STATUS_LEN];
    for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
        stat = readreg(fd, buf, REG_STATUS, REG_STATUS_LEN);
        if(stat) usleep(100000);
    }
    if(i == HANDSHAKE_TRIES) return false;
    if(buf[1] != 0xff){
        if(buf[5]){
            stat = 1;
            for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
                memset(buf, 0, sizeof(buf));
                buf[0] = REG_CLERR;
                stat = writereg(fd, buf, REG_CLERR_LEN);
                usleep(100000);
                if(!stat) stat = readreg(fd, buf, REG_STATUS, REG_STATUS_LEN);
                if(!stat && buf[5]) stat = 1;
            }
            if(i == HANDSHAKE_TRIES) return false;
        }
        readreg(fd, buf, REG_STATUS, REG_STATUS_LEN);
        if(buf[1] != 0xff){
            go_home(fd);
            readreg(fd, buf, REG_STATUS, REG_STATUS_LEN);
        }
    }
    return true;
}

// the same - but library function
bool wheel_clear_err(wheel_descr *w){
    return check_and_clear_err(w->fd);
}

static void get_props(wheel_descr *wheel){
    uint8_t buf[REG_NAME_LEN+1];
    int fd = wheel->fd;
    if(fd < 0){
        return;
    }
    check_and_clear_err(fd);
    // get status of wheel
    if(readreg(fd, buf, REG_INFO, REG_INFO_LEN)) return;
    wheel->ID = buf[5];
    wheel->maxpos = buf[4];
    DBG("Wheel with id '%c' and maxpos %d", wheel->ID, wheel->maxpos);
    char *getwname(int id){
        memset(buf, 0, sizeof(buf));
        buf[0] = REG_NAME;
        buf[1] = WHEEL_NAME;
        buf[2] = id;
        if(writereg(fd, buf, REG_NAME_LEN)) return NULL;
        if(readreg(fd, buf, REG_NAME, REG_NAME_LEN)) return NULL;
        if(buf[6]){
            char *x = strchr((char*)&buf[6], ' ');
            if(x) *x = 0;
            return (char*)&buf[6];
        }
        else return NULL;
    }
    char *nm = getwname(wheel->ID);
    if(nm){
        strncpy(wheel->name, nm, 9);
        DBG("Wheel name: %s", wheel->name);
    }
}


/**
 * Find turrets present
 * @param wheels (o) - if not NULL - list of wheels found (like "ABC") (allocated here!)
 * @param err (o)    - status code
 * @return amount of devices found
 *
 * WARNING! If there's more than one turret with wheels having same name
 *    access by wheel ID could lead undefined behaviour!
 */
int find_wheels(wheel_descr **wheels, wheel_error *err){
#define RETERR(x) do{if(err){*err = x;} return 0;}while(0)
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    wheel_descr *Found = NULL;
    if(err) *err = WHERR_ALLOK;
    // Create the udev object
    udev = udev_new();
    int N = 0;
    if(!udev){
        RETERR(WHERR_UDEV);
    }
    // Create a list of the devices in the 'hidraw' subsystem.
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    // Check out each device found
    udev_list_entry_foreach(dev_list_entry, devices){
        const char *path;
        struct udev_device *dev;
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        const char *devpath = udev_device_get_devnode(dev);
        DBG("Device Node Path: %s", devpath);
        dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if(!dev){
            fprintf(stderr, "Unable to find parent usb device for %s\n", devpath);
            udev_device_unref(dev);
            continue;
        }
        const char *vid, *pid;
        vid = udev_device_get_sysattr_value(dev,"idVendor");
        pid = udev_device_get_sysattr_value(dev, "idProduct");
        DBG("  VID/PID: %s/%s", vid, pid);
        if(strcmp(vid, WHEEL_VID) == 0 && strcmp(pid, WHEEL_PID) == 0){
            ++N;
            if(!Found){
                Found = MALLOC(wheel_descr, 1);
            }else{
                Found = realloc(Found, sizeof(wheel_descr)*N);
                if(!Found){
                    perror("realloc()");
                    exit(-1);
                }
            }
            wheel_descr *curdev = &Found[N-1];
            int fd = open(devpath, O_RDWR|O_NONBLOCK);
            if(fd < 0){
                if(err) *err = WHERR_CANTOPEN;
                curdev->fd = -1;
            }else
                curdev->fd = fd;
            DBG("%s  %s",
                    udev_device_get_sysattr_value(dev,"manufacturer"),
                    udev_device_get_sysattr_value(dev,"product"));
            curdev->serial = strdup(udev_device_get_sysattr_value(dev, "serial"));
            DBG("serial: %s\n", curdev->serial);
            get_props(curdev);
        }
        udev_device_unref(dev);
    }
    // Free the enumerator object
    udev_enumerate_unref(enumerate);
    if(wheels){
        *wheels = Found;
    }else
        free(Found);
    return N;
#undef RETERR
}

/**
 * @brief del_wheels - free memory of wheels descriptors
 * @param w - array of descriptors
 * @param N - length of w
 */
void del_wheels(wheel_descr *w, int N){
    if(N == 0) return;
    for(int i = 0; i < N; ++i){
        FREE(w[i].serial);
        close(w[i].fd);
    }
    FREE(w);
}


/**
 * @brief move_wheel - unblocking move given wheel
 * @param w          - wheel
 * @param filter_pos - target position
 * @return true if all OK
 */
bool move_wheel(wheel_descr *w, int filter_pos){
    int wheel_fd = w->fd;
    if(wheel_fd < 0) return false;
    if(filter_pos == poll_wheelstatus(wheel_fd)){
        DBG("Wheel %c is ON position (%d)", w->ID, filter_pos);
        return true; // @ target position
    }
    uint8_t buf[REG_GOTO_LEN];
    int i, stat = 1;
    for(i = 0; i < HANDSHAKE_TRIES && stat; ++i){
        DBG("i=%d",i);
        memset(buf, 0, REG_GOTO_LEN);
        buf[0] = REG_GOTO;
        buf[1] = filter_pos;
        stat = writereg(wheel_fd, buf, REG_GOTO_LEN);
        usleep(100000);
        if(stat) continue;
        if((stat = readreg(wheel_fd, buf, REG_GOTO, REG_GOTO_LEN))) continue;
        if(buf[1] != 0xff){
            stat = 1; continue;
        }else{
            readreg(wheel_fd, buf, REG_GOTO, REG_HOME_LEN);
            break;
        }
    }
    if(i == HANDSHAKE_TRIES) return false;
    //poll_wheelstatus(wheel_fd);
    //check_and_clear_err(wheel_fd);
    return true;
}
