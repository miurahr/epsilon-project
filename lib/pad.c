/*
 * $Id: pad.c,v 1.15 2010/02/05 23:50:22 simakov Exp $
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
#include <pad.h>
#include <color.h>

void extend_channel(unsigned char **input_channel,
                    coeff_t **output_channel,
                    int input_width, int input_height,
                    int output_width, int output_height)
{
    int i, j;

    /* Sanity checks */
    assert((input_width > 0) && (input_height > 0));
    assert((output_width > 0) && (output_height > 0));
    assert(output_width >= input_width);
    assert(output_height >= input_height);

    /* Copy original */
    for (i = 0; i < input_height; i++) {
        for (j = 0; j < input_width; j++) {
            output_channel[i][j] = input_channel[i][j];
        }
    }

    /* Fill horizontally */
    for (i = 0; i < input_height; i++) {
        for (j = 0; j < output_width - input_width; j++) {
            output_channel[i][input_width + j] =
                output_channel[i][ABS(input_width - j - 1)];
        }
    }

    /* Fill vertically */
    for (j = 0; j < output_width; j++) {
        for (i = 0; i < output_height - input_height; i++) {
            output_channel[i + input_height][j] =
                output_channel[ABS(input_height - i - 1)][j];
        }
    }
}

void extract_channel(coeff_t **input_channel,
                     unsigned char **output_channel,
                     int input_width, int input_height,
                     int output_width, int output_height)
{
    int i, j;

    /* Sanity checks */
    assert((input_width > 0) && (input_height > 0));
    assert((output_width > 0) && (output_height > 0));
    assert(output_width <= input_width);
    assert(output_height <= output_height);

    /* Extract & clip original data */
    for (i = 0; i < output_height; i++) {
        for (j = 0; j < output_width; j++) {
            output_channel[i][j] = CLIP(input_channel[i][j]);
        }
    }
}
