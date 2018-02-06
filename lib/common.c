/*
 * $Id: common.c,v 1.15 2010/02/05 23:50:22 simakov Exp $
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

int number_of_bits(int value)
{
    int bits;

    bits = NUMBER_OF_BITS(GET_BYTE(value, 3));
    if (bits) return bits + 24;

    bits = NUMBER_OF_BITS(GET_BYTE(value, 2));
    if (bits) return bits + 16;

    bits = NUMBER_OF_BITS(GET_BYTE(value, 1));
    if (bits) return bits + 8;

    bits = NUMBER_OF_BITS(GET_BYTE(value, 0));
    if (bits) return bits;

    return 0;
}

int is_power_of_two(int value)
{
    return (value == (1 << (number_of_bits(value) - 1)));
}
