/*
 * $Id: cobs.c,v 1.14 2010/02/05 23:50:22 simakov Exp $
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
#include <cobs.h>

#define FINISH_BLOCK(_x) (*code_ptr = (_x), \
                           code_ptr = output_data++, \
                           code = 0x01)

int stuff_data(unsigned char *input_data, unsigned char *output_data,
               int input_length, int output_length)
{
    unsigned char *input_end = input_data + input_length;
    unsigned char *output_start = output_data;

    unsigned char *code_ptr = output_data++;
    unsigned char  code = 0x01;

    /* Sanity checks */
    assert(input_length > 0);
    assert(output_length >= input_length + input_length / 254 + 1);

    while (input_data < input_end) {
        if (*input_data == 0) {
            FINISH_BLOCK(code);
        } else {
            *output_data++ = *input_data;
            code++;

            if (code == 0xff) {
                FINISH_BLOCK(code);
            }
        }

        input_data++;
    }

    FINISH_BLOCK(code);

    return output_data - output_start - 1;
}

int unstuff_data(unsigned char *input_data, unsigned char *output_data,
                 int input_length, int output_length)
{
    unsigned char *input_end = input_data + input_length;
    unsigned char *output_end = output_data + output_length;
    unsigned char *output_start = output_data;

    /* Sanity checks */
    assert(input_length > 0);
    assert(output_length >= input_length);

    while (input_data < input_end) {
        int code = *input_data++;
        int i;

        for (i = 1; (i < code) && (input_data < input_end) && (output_data < output_end); i++) {
            *output_data++ = *input_data++;
        }

        if ((code > 0x00) && (code < 0xff) &&
            (input_data < input_end) && (output_data < output_end)) {
            *output_data++ = 0;
        }
    }

    return output_data - output_start;
}
