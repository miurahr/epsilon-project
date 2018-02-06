/*
 * $Id: merge_split.c,v 1.15 2010/02/05 23:50:22 simakov Exp $
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
#include <merge_split.h>

void merge_channels(unsigned char *channel_A, unsigned char *channel_B,
                    unsigned char *channel_AB, int len_A, int len_B)
{
    int pkg_A, pkg_B; /* Real number of bytes in the package */
    int div_A, div_B; /* Base number of bytes in the package */
    int rem_A, rem_B; /* Instant reminder */
    int cur_A, cur_B; /* Current savings */
    int tr0_A, tr0_B; /* Lower threshold */
    int tr1_A, tr1_B; /* Upper threshold */
    int i, pkg, min;

    /* Sanity checks. I love them! */
    assert((len_A > 0) && (len_B > 0));

    /* Total number of packages */
    min = MIN(len_A, len_B);

    /* 1-st channel setup */
    div_A = len_A / min;
    cur_A = len_A;
    rem_A = 0;
    tr0_A = div_A * min;
    tr1_A = tr0_A + min;

    /* 2-nd channel setup */
    div_B = len_B / min;
    cur_B = len_B;
    rem_B = 0;
    tr0_B = div_B * min;
    tr1_B = tr0_B + min;

    /* Loop for all packages */
    for (pkg = 0; pkg < min; pkg++) {
        /* Update current savings */
        cur_A = len_A + rem_A;

        if (cur_A >= tr1_A) {
            /* Choose maximal package & update reminder */
            pkg_A = div_A + 1;
            rem_A = cur_A - tr1_A;
        } else {
            /* Choose minimal package & update reminder */
            pkg_A = div_A;
            rem_A = cur_A - tr0_A;
        }

        /* Update current savings */
        cur_B = len_B + rem_B;

        if (cur_B >= tr1_B) {
            /* Choose maximal package & update reminder */
            pkg_B = div_B + 1;
            rem_B = cur_B - tr1_B;
        } else {
            /* Choose minimal package & update reminder */
            pkg_B = div_B;
            rem_B = cur_B - tr0_B;
        }

        /* Put bytes from the 1-st channel */
        for (i = 0; i < pkg_A; i++) {
            *channel_AB++ = *channel_A++;
        }

        /* Put bytes from the 2-nd channel */
        for (i = 0; i < pkg_B; i++) {
            *channel_AB++ = *channel_B++;
        }
    }
}

void split_channels(unsigned char *channel_AB,
                    unsigned char *channel_A, unsigned char *channel_B,
                    int len_AB, int len_A, int len_B,
                    int *real_len_A, int *real_len_B)
{
    int pkg_A, pkg_B; /* Real number of bytes in the package */
    int div_A, div_B; /* Base number of bytes in the package */
    int rem_A, rem_B; /* Instant reminder */
    int cur_A, cur_B; /* Current savings */
    int tr0_A, tr0_B; /* Lower threshold */
    int tr1_A, tr1_B; /* Upper threshold */
    int i, pkg, min;

    unsigned char *end_AB;
    unsigned char *end_A;
    unsigned char *end_B;

    /* Sanity checks */
    assert((len_A > 0) && (len_B > 0) && (len_AB > 0));
    assert(real_len_A && real_len_B);
    assert(len_A + len_B >= len_AB);

    /* Set end-of-buffer pointers */
    end_AB = channel_AB + len_AB;
    end_A = channel_A + len_A;
    end_B = channel_B + len_B;

    /* Real amount of saved bytes */
    *real_len_A = *real_len_B = 0;

    /* Original number of packages (real stream may be truncated) */
    min = MIN(len_A, len_B);

    /* 1-st channel setup */
    div_A = len_A / min;
    cur_A = len_A;
    rem_A = 0;
    tr0_A = div_A * min;
    tr1_A = tr0_A + min;

    /* 2-nd channel setup */
    div_B = len_B / min;
    cur_B = len_B;
    rem_B = 0;
    tr0_B = div_B * min;
    tr1_B = tr0_B + min;

    /* Loop for all packages */
    for (pkg = 0; pkg < min; pkg++) {
        /* Update current savings */
        cur_A = len_A + rem_A;

        if (cur_A >= tr1_A) {
            /* Choose maximal package & update reminder */
            pkg_A = div_A + 1;
            rem_A = cur_A - tr1_A;
        } else {
            /* Choose minimal package & update reminder */
            pkg_A = div_A;
            rem_A = cur_A - tr0_A;
        }

        /* Update current savings */
        cur_B = len_B + rem_B;

        if (cur_B >= tr1_B) {
            /* Choose maximal package & update reminder */
            pkg_B = div_B + 1;
            rem_B = cur_B - tr1_B;
        } else {
            /* Choose minimal package & update reminder */
            pkg_B = div_B;
            rem_B = cur_B - tr0_B;
        }

        /* Extract pkg_A bytes from the stream into the channel_A */
        for (i = 0; (i < pkg_A) && (channel_A < end_A) && (channel_AB < end_AB); i++) {
            *channel_A++ = *channel_AB++;
            (*real_len_A)++;
        }

        /* Extract pkg_B bytes from the stream into the channel_B */
        for (i = 0; (i < pkg_B) && (channel_B < end_B) && (channel_AB < end_AB); i++) {
            *channel_B++ = *channel_AB++; 
            (*real_len_B)++;
        }

        /* All avaiable bytes are processed */
        if (channel_AB >= end_AB) {
            assert(*real_len_A + *real_len_B == len_AB);

            if ((pkg == min - 1) && (channel_A == end_A) && (channel_B == end_B)) {
                assert((*real_len_A == len_A) && (*real_len_B == len_B));
            }

            return;
        }
    }
}
