
/*
 * The olsr.org Optimized Link-State Routing daemon version 2 (olsrd2)
 * Copyright (c) 2004-2013, the olsr.org team - see HISTORY file
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <stddef.h>

/* support EXPORT macro of OONF */
#ifndef EXPORT
#  define EXPORT __attribute__((visibility ("default")))
#endif

/* give everyone an arraysize implementation */
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)  (sizeof(a) / sizeof(*(a)))
#endif

/* convert the value into a string */
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

/*
 * This force gcc to always inline, which prevents errors
 * with option -Os
 */
#ifndef inline
#ifdef __GNUC__
#define inline inline __attribute__((always_inline))
#else
#define inline inline
#endif
#endif

/* printf size_t modifiers*/
#if defined(WIN32)
  #define PRINTF_SIZE_T_SPECIFIER     "Iu"
  #define PRINTF_SIZE_T_HEX_SPECIFIER "Ix"
  #define PRINTF_SSIZE_T_SPECIFIER    "Id"
  #define PRINTF_PTRDIFF_T_SPECIFIER  "Id"
#elif defined(RIOT_VERSION)
  #define PRINTF_SIZE_T_SPECIFIER      "d"
  #define PRINTF_SIZE_T_HEX_SPECIFIER  "x"
  #define PRINTF_SSIZE_T_SPECIFIER     "d"
  #define PRINTF_PTRDIFF_T_SPECIFIER   "d"
#elif defined(__GNUC__)
  #define PRINTF_SIZE_T_SPECIFIER     "zu"
  #define PRINTF_SIZE_T_HEX_SPECIFIER "zx"
  #define PRINTF_SSIZE_T_SPECIFIER    "zd"
  #define PRINTF_PTRDIFF_T_SPECIFIER  "zd"
#else
  // TODO figure out which to use.
  #error Please implement size_t modifiers
#endif

#include <limits.h>

/* we have C99 ? */
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#include <stdbool.h>
#else

/*
 * This include file creates stdint/stdbool datatypes for
 * visual studio, because microsoft does not support C99
 */

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

/* Minimum of signed integral types.  */
#ifndef INT8_MIN
# define INT8_MIN   (-128)
#endif
#ifndef INT16_MIN
# define INT16_MIN    (-32767-1)
#endif
#ifndef INT32_MIN
# define INT32_MIN    (-2147483647-1)
#endif

/* Maximum of signed integral types.  */
#ifndef INT8_MAX
# define INT8_MAX   (127)
#endif
#ifndef INT16_MAX
# define INT16_MAX    (32767)
#endif
#ifndef INT32_MAX
# define INT32_MAX    (2147483647)
#endif

/* Maximum of unsigned integral types.  */
#ifndef UINT8_MAX
# define UINT8_MAX    (255)
#endif
#ifndef UINT16_MAX
# define UINT16_MAX   (65535)
#endif
#ifndef UINT32_MAX
# define UINT32_MAX   (4294967295U)
#endif

/* printf modifier for int64_t and uint64_t */
#ifndef PRId64
#define PRId64        "lld"
#endif
#ifndef PRIu64
#define PRIu64        "llu"
#endif

#ifdef __GNUC__
/* we simulate a C99 environment */
#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#else
#error No boolean available, please extende common/common_types.h
#endif /* __GNUC__ */

#endif /* __STDC_VERSION__ && __STDC_VERSION__ >= 199901L */

#endif /* COMMON_TYPES_H_ */
