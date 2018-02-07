/*
 * $Id: common.h,v 1.30 2010/02/05 23:50:22 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

/** \file
 *
 *  \brief Useful macro and defines
 *
 *  This file contains useful macro and defines which are
 *  common to all library parts. */

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup misc Miscellanea */
/*@{*/

#ifdef _MSC_VER
#if _MSC_VER < 1900  // VS2015/17 provides snprintf
#include <stdio.h>
#include <stdarg.h>
/* Want safe, 'n += snprintf(b + n ...)' like function. If cp_max_len is 1
* then assume cp is pointing to a null char and do nothing. Returns number
* number of chars placed in cp excluding the trailing null char. So for
* cp_max_len > 0 the return value is always < cp_max_len; for cp_max_len
* <= 0 the return value is 0 (and no chars are written to cp). */
static int snprintf(char * cp, int cp_max_len, const char * fmt, ...)
{
    va_list args;
    int n;

    if (cp_max_len < 2)
        return 0;
    va_start(args, fmt);
    n = vsnprintf(cp, cp_max_len, fmt, args);
    va_end(args);
    return (n < cp_max_len) ? n : (cp_max_len - 1);
}
#endif  // _MSC_VER < 1900
#endif  // _MSC_VER

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

/** Maximum value */
#define MAX(_x, _y)             ((_x) > (_y) ? (_x) : (_y))
/** Minimum value */
#define MIN(_x, _y)             ((_x) < (_y) ? (_x) : (_y))
/** Absolute value */
#define ABS(_x)                 ((_x) >= 0 ? (_x) : -(_x))
/** Square root */
#define SQRT2                   1.414213562373095
/** Very helpful definition */
#define local                   static

/** Table to speed-up number_of_bits() calculation */
local int number_of_bits_table[256] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/** Number of bits in a byte value */
#define NUMBER_OF_BITS(_x) (number_of_bits_table[(_x)])
/** Extract one byte from integer */
#define GET_BYTE(_x, _i) (((unsigned char *) &(_x))[(_i)])

/** Type definition for filter coefficients */
typedef double coeff_t;

/** Number of bits in the value
 *
 *  This function computes the number of bits in the \a value
 *  (e.g. number_of_bits(13) = 4).
 *
 *  \param value Target value
 *
 *  \return Number of bits */
int number_of_bits(int value);

/** Check whether the \a value is a power of two or not
 *
 *  This function checks whether the \a value is a
 *  power of two or not.
 *
 *  \param value Target value
 *
 *  \return \c 1 if \a value is a power of two and \c 0 otherwise */
int is_power_of_two(int value);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_H__ */
