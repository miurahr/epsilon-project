/*
 * $Id: misc.c,v 1.11 2010/02/05 23:50:22 simakov Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <misc.h>
#include <stdio.h>
#include <stdlib.h>

/* Check that the value is a power of two */
int power_of_two(int value)
{
    int tmp = value;
    int bits = 0;

    while (tmp > 0) {
        tmp >>= 1;
        bits++;
    }

    return (value == (1 << (bits - 1)));
}

/* Get number of files in the list */
int get_number_of_files(char **files)
{
    int n;

    if (!files) {
        return 0;
    }

    for (n = 0; *files++; n++);

    return n;
}

/* Format time for progress indicator */
char *format_time(int delta, char *buf, int nbytes) {
    if (delta >= 24 * 60 * 60) {
        snprintf(buf, nbytes, "%.2f days",
            (double) delta /  (24.0 * 60.0 * 60.0));
    } else if (delta >= 60 * 60) {
        snprintf(buf, nbytes, "%.2f hours",
            (double) delta /  (60.0 * 60.0));
    } else if (delta >= 60) {
        snprintf(buf, nbytes, "%.2f minutes",
            (double) delta /  60.0);
    } else if (delta >= 0 && delta < 60){
        snprintf(buf, nbytes, "%.2f seconds",
            (double) delta);
    } else {
        snprintf(buf, nbytes, "0.00 seconds");
    }

    return buf;
}

/* Print blank line */
void print_blank_line(int len) {
    int i;

    for (i = 0; i < len; i++) {
        printf(" ");
    }

    printf("\r");
    fflush(stdout);
}


/* Flatten 2D-array */
void transform_2D_to_1D(unsigned char **src, unsigned char *dst, int w, int h) {
    int i, j;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            *dst++ = src[i][j];
        }
    }
}

/* Expand 1D-array */
void transform_1D_to_2D(unsigned char *src, unsigned char **dst, int w, int h) {
    int i, j;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            dst[i][j] = *src++;
        }
    }
}
