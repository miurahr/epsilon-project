/*
 * $Id: make_number_of_bits_table.c,v 1.3 2010/02/05 23:50:23 simakov Exp $
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

#include <stdio.h>

#define ALIGN_MASK 0x0f

int number_of_bits(int value)
{
    int bits = 0;

    while (value > 0) {
        value >>= 1;
        bits++;
    }

    return bits;
}

void make_number_of_bits_table()
{
    int i;

    printf("local int number_of_bits_table[256] = {\n");

    for (i = 0; i < 256; i++) {
        int bit_length = number_of_bits(i);

        if ((i & ALIGN_MASK) == 0) {
            printf("    ");
        }

        printf("%d,%s", bit_length, (i & ALIGN_MASK) == ALIGN_MASK ? "" : " ");

        if ((i & ALIGN_MASK) == ALIGN_MASK) {
            printf("\n");
        }
    }

    printf("};\n");
}

int main(int argc, char **argv)
{
    make_number_of_bits_table();
    return 0;
}
