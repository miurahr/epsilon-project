/*
 * $Id: filter.c,v 1.21 2010/04/05 06:11:55 simakov Exp $
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
#include <filter.h>
#include <filterbank.h>
#include <daub97lift.h>
#include <mem_alloc.h>
#include <string.h>

inline local int periodic_extension(int index, int length)
{
    if (index >= 0) {
        if (index < length) {
            return index;
        } else {
            return (index % length);
        }
    } else {
        return (length - 1 - (ABS(index) - 1) % length);
    }
}

inline local int symmetric_W_extension(int index, int length)
{
    if ((index >= 0) && (index < length)) {
        return index;
    }

    if (length == 1) {
        return 0;
    }

    index = ABS(index) % (2 * length - 2);

    if (index >= length) {
        index = 2 * length - 2 - index;
    }

    return index;
}

inline local int symmetric_H_extension(int index, int length)
{
    if ((index >= 0) && (index < length)) {
        return index;
    }

    if (length == 1) {
        return 0;
    }

    index = (ABS(index) - (index < 0)) % (2 * length);

    if (index >= length) {
        index = 2 * length - index - 1;
    }

    return index;
}

/* Note: output_length parameter is not actually used but it's preserved
 * in order to make downsample_signal() symmetric to and upsample_signal(). */
inline local void downsample_signal(coeff_t *input_signal, coeff_t *output_signal,
                                    int input_length, int output_length, int phase)
{
    int i, j;

    for (i = phase, j = 0; i < input_length; i += 2, j++) {
        output_signal[j] = input_signal[i];
    }
}

inline local void upsample_signal(coeff_t *input_signal, coeff_t *output_signal,
                                  int input_length, int output_length, int phase)
{
    int i, j, k;

    for (i = 0, j = phase; i < input_length; i++, j += 2) {
        output_signal[j] = input_signal[i];
    }

    for (k = phase ^ 1; k < output_length; k += 2) {
        output_signal[k] = 0;
    }
}

inline local void filter_periodic(coeff_t *input_signal, coeff_t *output_signal,
                                  int signal_length, filter_t *filter)
{
    int i, j, k;

    switch (filter->causality) {
        case CAUSAL:
        {
            for (i = 0; i < signal_length; i += 1) {
                output_signal[i] = 0;
                for (j = 0; j < filter->length; j++) {
                    k = periodic_extension(i - j, signal_length);
                    output_signal[i] += input_signal[k] * filter->coeffs[j];
                }
            }

            break;
        }
        case ANTICAUSAL:
        {
            for (i = 0; i < signal_length; i += 2) {
                output_signal[i] = 0;
                for (j = 0; j < filter->length; j++) {
                    k = periodic_extension(i + j, signal_length);
                    output_signal[i] +=
                        input_signal[k] * filter->coeffs[filter->length - j - 1];
                }
            }

            break;
        }
        default:
        {
            assert(0);
            break;
        }
    }
}

inline local void filter_symmetric(coeff_t *input_signal, coeff_t *output_signal,
                                   int signal_length, filter_t *filter)
{
    int i, j, k1, k2;

    switch (filter->causality) {
        case SYMMETRIC_WHOLE:
        {
            if (filter->type == LOWPASS_ANALYSIS) {
                for (i = 0; i < signal_length; i += 2) {
                    output_signal[i] = input_signal[i] * filter->coeffs[0];
                    for (j = 1; j < filter->length; j++) {
                        k1 = symmetric_W_extension(i + j, signal_length);
                        k2 = symmetric_W_extension(i - j, signal_length);
                        output_signal[i] +=
                            (input_signal[k1] + input_signal[k2]) * filter->coeffs[j];
                    }
                }
            } else if (filter->type == HIGHPASS_ANALYSIS) {
                for (i = 1; i < signal_length; i += 2) {
                    output_signal[i] = input_signal[i] * filter->coeffs[0];
                    for (j = 1; j < filter->length; j++) {
                        k1 = symmetric_W_extension(i + j, signal_length);
                        k2 = symmetric_W_extension(i - j, signal_length);
                        output_signal[i] +=
                            (input_signal[k1] + input_signal[k2]) * filter->coeffs[j];
                    }
                }
            } else {
                for (i = 0; i < signal_length; i++) {
                    output_signal[i] = input_signal[i] * filter->coeffs[0];
                    for (j = 1; j < filter->length; j++) {
                        k1 = symmetric_W_extension(i + j, signal_length);
                        k2 = symmetric_W_extension(i - j, signal_length);
                        output_signal[i] +=
                            (input_signal[k1] + input_signal[k2]) * filter->coeffs[j];
                    }
                }
            }

            break;
        }
        /* Some day I hope to add 'case SYMMETRIC_HALF' here */
        default:
        {
            assert(0);
            break;
        }
    }
}

local void analysis_1D(coeff_t *input_signal, coeff_t *output_signal,
                       coeff_t *temp, int signal_length, filterbank_t *fb)
{
    coeff_t *lowpass;
    coeff_t *highpass;

    /* Sanity checks */
    assert(signal_length > 0);
    assert((fb->type == BIORTHOGONAL) || ((fb->type == ORTHOGONAL)
            && !(signal_length & 1)));

    /* Trivial case */
    if (signal_length == 1) {
        output_signal[0] = input_signal[0] * SQRT2;
        return;
    }

    if (fb->type == ORTHOGONAL) {
        lowpass = output_signal;
        highpass = output_signal + signal_length / 2;

        /* Lowpass analysis */
        filter_periodic(input_signal, temp, signal_length, fb->lowpass_analysis);
        downsample_signal(temp, lowpass, signal_length, signal_length / 2, PHASE_EVEN);

        /* Highpass analysis */
        filter_periodic(input_signal, temp, signal_length, fb->highpass_analysis);
        downsample_signal(temp, highpass, signal_length, signal_length / 2, PHASE_EVEN);
    } else {
        lowpass = output_signal;
        highpass = output_signal + signal_length / 2 + (signal_length & 1);

        /* Lowpass analysis */
        filter_symmetric(input_signal, temp, signal_length, fb->lowpass_analysis);
        downsample_signal(temp, lowpass, signal_length, (signal_length + 1) / 2, PHASE_EVEN);

        /* Highpass analysis */
        filter_symmetric(input_signal, temp, signal_length, fb->highpass_analysis);
        downsample_signal(temp, highpass, signal_length, signal_length / 2, PHASE_ODD);
    }
}

local void synthesis_1D(coeff_t *input_signal, coeff_t *output_signal,
                        coeff_t *temp1, coeff_t *temp2, coeff_t *temp3,
                        int signal_length, filterbank_t *fb)
{
    coeff_t *lowpass;
    coeff_t *highpass;
    int i;

    /* Sanity checks */
    assert(signal_length > 0);
    assert((fb->type == BIORTHOGONAL) || ((fb->type == ORTHOGONAL)
            && !(signal_length & 1)));

    /* Trivial case */
    if (signal_length == 1) {
        output_signal[0] = input_signal[0] / SQRT2;
        return;
    }

    if (fb->type == ORTHOGONAL) {
        lowpass = input_signal;
        highpass = input_signal + signal_length / 2;

        /* Lowpass synthesis */
        upsample_signal(lowpass, temp1, signal_length / 2, signal_length, PHASE_EVEN);
        filter_periodic(temp1, temp2, signal_length, fb->lowpass_synthesis);

        /* Highpass synthesis */
        upsample_signal(highpass, temp1, signal_length / 2, signal_length, PHASE_EVEN);
        filter_periodic(temp1, temp3, signal_length, fb->highpass_synthesis);
    } else {
        lowpass = input_signal;
        highpass = input_signal + signal_length / 2 + (signal_length & 1);

        /* Lowpass synthesis */
        upsample_signal(lowpass, temp1, (signal_length + 1) / 2, signal_length, PHASE_EVEN);
        filter_symmetric(temp1, temp2, signal_length, fb->lowpass_synthesis);

        /* Highpass synthesis */
        upsample_signal(highpass, temp1, signal_length / 2, signal_length, PHASE_ODD);
        filter_symmetric(temp1, temp3, signal_length, fb->highpass_synthesis);
    }

    /* Combine arrays */
    for (i = 0; i < signal_length; i++) {
        output_signal[i] = temp2[i] + temp3[i];
    }
}

void analysis_2D(coeff_t **input_signal, coeff_t **output_signal,
                 int signal_length, int mode, filterbank_t *fb)
{
    coeff_t *input;
    coeff_t *output;
    coeff_t *temp;

    int scale, length;
    int scales;
    int i, j;

    assert(signal_length > 1);

    /* Transform as many times as possible */
    scales = number_of_bits(signal_length) - 1;

    /* Sanity checks */
    assert(((mode == MODE_NORMAL) && (signal_length == 1 << scales)) ||
           ((mode == MODE_OTLPF) && (signal_length == (1 << scales) + 1)));

    input = xmalloc(signal_length * sizeof(coeff_t));
    output = xmalloc(signal_length * sizeof(coeff_t));
    temp = xmalloc(signal_length * sizeof(coeff_t));

    for (i = 0; i < signal_length; i++) {
        for (j = 0; j < signal_length; j++) {
            output_signal[i][j] = input_signal[i][j];
        }
    }

    /* Transform image */
    for (scale = 0; scale < scales; scale++) {
        length = mode + (1 << (scales - scale));

        /* Transform rows */
        for (i = 0; i < length; i++) {
            for (j = 0; j < length; j++) {
                input[j] = output_signal[i][j];
            }

            if (!strcmp(fb->id, "daub97lift")) {
                if (length % 2) {
                    daub97lift_analysis_1D_odd(input, output, length);
                } else {
                    daub97lift_analysis_1D_even(input, output, length);
                }
            } else {
                analysis_1D(input, output, temp, length, fb);
            }

            for (j = 0; j < length; j++) {
                output_signal[i][j] = output[j];
            }
        }

        /* Transform columns */
        for (i = 0; i < length; i++) {
            for (j = 0; j < length; j++) {
                input[j] = output_signal[j][i];
            }

            if (!strcmp(fb->id, "daub97lift")) {
                if (length % 2) {
                    daub97lift_analysis_1D_odd(input, output, length);
                } else {
                    daub97lift_analysis_1D_even(input, output, length);
                }
            } else {
                analysis_1D(input, output, temp, length, fb);
            }

            for (j = 0; j < length; j++) {
                output_signal[j][i] = output[j];
            }
        }
    }

    free(input);
    free(output);
    free(temp);
}

void synthesis_2D(coeff_t **input_signal, coeff_t **output_signal,
                  int signal_length, int mode, filterbank_t *fb)
{
    coeff_t *input;
    coeff_t *output;
    coeff_t *temp1;
    coeff_t *temp2;
    coeff_t *temp3;

    int scale, length;
    int scales;
    int i, j;

    assert(signal_length > 1);

    /* Transform as many times as possible */
    scales = number_of_bits(signal_length) - 1;

    /* Sanity checks */
    assert(((mode == MODE_NORMAL) && (signal_length == 1 << scales)) ||
           ((mode == MODE_OTLPF) && (signal_length == (1 << scales) + 1)));

    /* Temporary arrays */
    input = xmalloc(signal_length * sizeof(coeff_t));
    output = xmalloc(signal_length * sizeof(coeff_t));
    temp1 = xmalloc(signal_length * sizeof(coeff_t));
    temp2 = xmalloc(signal_length * sizeof(coeff_t));
    temp3 = xmalloc(signal_length * sizeof(coeff_t));

    for (i = 0; i < signal_length; i++) {
        for (j = 0; j < signal_length; j++) {
            output_signal[i][j] = input_signal[i][j];
        }
    }

    /* Transform image */
    for (scale = 0; scale < scales; scale++) {
        length = mode + (1 << (scale + 1));

        /* Transform rows */
        for (i = 0; i < length; i++) {
            for (j = 0; j < length; j++) {
                input[j] = output_signal[i][j];
            }

            if (!strcmp(fb->id, "daub97lift")) {
                if (length % 2) {
                    daub97lift_synthesis_1D_odd(input, output, length);
                } else {
                    daub97lift_synthesis_1D_even(input, output, length);
                }
            } else {
                synthesis_1D(input, output, temp1, temp2, temp3, length, fb);
            }

            for (j = 0; j < length; j++) {
                output_signal[i][j] = output[j];
            }
        }

        /* Transform columns */
        for (i = 0; i < length; i++) {
            for (j = 0; j < length; j++) {
                input[j] = output_signal[j][i];
            }

            if (!strcmp(fb->id, "daub97lift")) {
                if (length % 2) {
                    daub97lift_synthesis_1D_odd(input, output, length);
                } else {
                    daub97lift_synthesis_1D_even(input, output, length);
                }
            } else {
                synthesis_1D(input, output, temp1, temp2, temp3, length, fb);
            }

            for (j = 0; j < length; j++) {
                output_signal[j][i] = output[j];
            }
        }
    }

    /* Release temporary arrays */
    free(input);
    free(output);
    free(temp1);
    free(temp2);
    free(temp3);
}
