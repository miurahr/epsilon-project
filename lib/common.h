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

/* Use _snprintf instead of snprintf under MSVC compiler */
#if defined(_WIN32) && !defined(__MINGW32__)
#define snprintf    _snprintf
#endif

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
