/*
 * $Id: dc_level.c,v 1.11 2010/02/05 23:50:22 simakov Exp $
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
#include <dc_level.h>

coeff_t dc_level_shift(coeff_t **channel, int width, int height)
{
    int i, j;
    coeff_t average = 0.0;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            average += channel[i][j];
        }
    }

    average = average / (width * height);

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            channel[i][j] -= average;
        }
    }

    return average;
}

void dc_level_unshift(coeff_t **channel, coeff_t average, int width, int height)
{
    int i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            channel[i][j] += average;
        }
    }
}
