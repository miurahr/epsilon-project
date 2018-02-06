/*
 * $Id: resample.c,v 1.11 2010/02/05 23:50:22 simakov Exp $
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
#include <resample.h>

void bilinear_resample_channel(coeff_t **input_channel, coeff_t **output_channel,
                               int input_width, int input_height,
                               int output_width, int output_height)
{
    coeff_t t, u;
    coeff_t tmp;

    int i, j;
    int l, c;

    /* Sanity checks */
    assert((input_width > 1) && (input_height > 1));
    assert((output_width > 1) && (output_height > 1));

    for (i = 0; i < output_height; i++) {
        for (j = 0; j < output_width; j++) {
            tmp = (double) (input_height - 1) *
                ((double) i / (double) (output_height - 1));

            l = (int) tmp;

            if (l < 0) {
                l = 0;
            } else if (l >= input_height - 1) {
                l = input_height - 2;
            }

            u = tmp - (double) l;

            tmp = (double) (input_width - 1) *
                ((double) j / (double) (output_width - 1));

            c = (int) tmp;

            if (c < 0) {
                c = 0;
            } else if (c >= input_width - 1) {
                c = input_width - 2;
            }

            t = tmp - (double) c;

            output_channel[i][j] =
                input_channel[l][c] * (1 - t) * (1 - u) +
                input_channel[l + 1][c] * (1 - t) * u +
                input_channel[l][c + 1] * t * (1 - u) +
                input_channel[l + 1][c + 1] * t * u;
        }
    }
}
