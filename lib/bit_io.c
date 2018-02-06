/*
 * $Id: bit_io.c,v 1.14 2010/02/05 23:50:21 simakov Exp $
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

#include <common.h>
#include <bit_io.h>

void init_bits(bit_buffer *bb, unsigned char *buf, int size)
{
    assert(size > 0);

    bb->start = bb->next = buf;
    bb->end = bb->start + size;
    bb->bits = bb->pending = 0;
}

int write_bits(bit_buffer *bb, int value, int size)
{
    assert(size <= 24);

    /* No more space available */
    if (bb->next >= bb->end) {
        return BIT_BUFFER_OVERFLOW;
    }

    /* Save requested number of bits */
    bb->bits |= (value << bb->pending);
    bb->pending += size;

    /* Write complete octets */
    while (bb->pending >= 8) {
        if (bb->next >= bb->end) {
            return BIT_BUFFER_OVERFLOW;
        }

        *bb->next++ = (unsigned char) (bb->bits & 0xff);

        bb->bits >>= 8;
        bb->pending -= 8;
    }

    return BIT_BUFFER_OK;
}

int read_bits(bit_buffer *bb, int *value, int size)
{
    assert(size <= 24);

    /* Read missing bytes */
    while (bb->pending < size) {
        if (bb->next >= bb->end) {
            return BIT_BUFFER_UNDERFLOW;
        }

        bb->bits |= (*bb->next++ << bb->pending);
        bb->pending += 8;
    }

    *value = bb->bits & (~(~0 << size));

    bb->bits >>= size;
    bb->pending -= size;

    return BIT_BUFFER_OK;
}

int flush_bits(bit_buffer *bb)
{
    if (bb->pending) {
        return write_bits(bb, 0UL, 8);
    } else {
        return BIT_BUFFER_OK;
    }
}
