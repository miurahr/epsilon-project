/*
 * $Id: bit_io.h,v 1.17 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Bit I/O
 *
 *  This file contains bit I/O routines. */

#ifndef __BIT_IO_H__
#define __BIT_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup bit_io Bit I/O */
/*@{*/

#include <common.h>

/** All bits are successfuly processed */
#define BIT_BUFFER_OK           0
/** Cannot write bits, output buffer is full */
#define BIT_BUFFER_OVERFLOW     1
/** Cannot read bits, input buffer is empty */
#define BIT_BUFFER_UNDERFLOW    2

/** Bit-buffer structure
 *
 *  This structure represents bit-buffer. */
typedef struct bit_buffer_tag {
    /** Start of buffer */
    unsigned char *start;
    /** End of buffer */
    unsigned char *end;
    /** Next input/output byte */
    unsigned char *next;
    /** Bit buffer */
    unsigned long bits;
    /** Pending bits */
    int pending;
} bit_buffer;

/** Write one zero bit */
#define write_0(_bb)            write_bits(_bb, 0UL, 1)
/** Write one unity bit */
#define write_1(_bb)            write_bits(_bb, 1UL, 1)
/** Read one bit */
#define read_bit(_bb, _bit)     read_bits(_bb, _bit, 1)

/** Initialize bit-buffer for reading or writting
 *
 *  This function initializes bit-buffer \a bb for reading or
 *  writting bits.
 *
 *  \param bb Bit-buffer to initialize
 *  \param buf Actual input/output buffer
 *  \param size Actual buffer size
 *
 *  \return \c VOID */
void init_bits(bit_buffer *bb, unsigned char *buf, int size);

/** Write bits
 *
 *  This function writes \a size least significant bits of
 *  the \a value to the bit-buffer \a bb.
 *
 *  \note The function expects that \a size <= 24.
 *
 *  \param bb Bit-buffer
 *  \param value Bits to write
 *  \param size Number of bits to write
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
int write_bits(bit_buffer *bb, int value, int size);

/** Read bits
 *
 *  This function reads \a size least significant bits
 *  from the bit-buffer \a bb  into the \a value argument.
 *
 *  \note Function expects that \a size <= 24.
 *
 *  \param bb Bit-buffer
 *  \param value Location to store the bits
 *  \param size Number of bits to read
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
int read_bits(bit_buffer *bb, int *value, int size);

/** Flush bits
 *
 *  This function flushes all pending bits if there are any.
 *
 *  \param bb Bit-buffer
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
int flush_bits(bit_buffer *bb);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __BIT_IO_H__ */
