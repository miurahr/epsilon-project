/*
 * $Id: make_RGB_to_YCbCr_table.c,v 1.3 2010/02/05 23:50:23 simakov Exp $
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

#define ALIGN_MASK 0x07

int number_of_bits(int value)
{
    int bits = 0;

    while (value > 0) {
        value >>= 1;
        bits++;
    }

    return bits;
}

void make_RGB_to_YCbCr_table(double factor, char *name)
{
    int i;

    printf("local double %s[256] = {\n", name);

    for (i = 0; i < 256; i++) {
        double value = factor * i;

        if ((i & ALIGN_MASK) == 0) {
            printf("    ");
        }

        printf("%8g,%s", value, (i & ALIGN_MASK) == ALIGN_MASK ? "" : " ");

        if ((i & ALIGN_MASK) == ALIGN_MASK) {
            printf("\n");
        }
    }

    printf("};\n\n");
}

int main(int argc, char **argv)
{
    double factors[] = {
        0.299000, 0.587000, 0.114000, 0.168736,
        0.331264, 0.500000, 0.418688, 0.081312,
    };

    char *names[] = {
        "O_299000", "O_587000", "O_114000", "O_168736",
        "O_331264", "O_500000", "O_418688", "O_081312",
    };

    int length = sizeof(factors) / sizeof(factors[0]);
    int i;

    for (i = 0; i < length; i++) {
        make_RGB_to_YCbCr_table(factors[i], names[i]);
    }

    return 0;
}
