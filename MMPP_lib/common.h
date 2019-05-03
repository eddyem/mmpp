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
// common macros
#pragma once
#ifndef COMMON_H__
#define COMMON_H__

#include <stdio.h>
#include <stdlib.h>

// unused arguments of functions
#define _U_         __attribute__((__unused__))
// break absent in `case`
#define FALLTHRU    __attribute__ ((fallthrough))
// and synonym for FALLTHRU
#define NOBREAKHERE __attribute__ ((fallthrough))
// weak functions
#define WEAK        __attribute__ ((weak))

#define COLOR_RED           "\033[1;31;40m"
#define COLOR_GREEN         "\033[1;32;40m"
#define COLOR_OLD           "\033[0;0;0m"
/*
 * print function name, debug messages
 * debug mode, -DEBUG
 */
#ifdef EBUG
    #define FNAME() do{ fprintf(stderr, COLOR_OLD); \
        fprintf(stderr, "\n%s (%s, line %d)\n", __func__, __FILE__, __LINE__);} while(0)
    #define DBG(...) do{ fprintf(stderr, COLOR_OLD); \
                    fprintf(stderr, "%s (%s, line %d): ", __func__, __FILE__, __LINE__); \
                    fprintf(stderr, __VA_ARGS__);           \
                    fprintf(stderr, "\n");} while(0)
#else
    #define FNAME()  do{}while(0)
    #define DBG(...) do{}while(0)
#endif //EBUG

/*
 * Memory allocation
 */
#define ALLOC(type, var, size)  type * var = ((type *)my_alloc(size, sizeof(type)))
#define MALLOC(type, size) ((type *)my_alloc(size, sizeof(type)))
#define FREE(ptr)  do{if(ptr){free(ptr); ptr = NULL;}}while(0)

#ifndef DBL_EPSILON
#define DBL_EPSILON        (2.2204460492503131e-16)
#endif

void *my_alloc(size_t N, size_t S);

#endif // COMMON_H__
