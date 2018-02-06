/*
 * $Id: daub97lift.h,v 1.4 2010/02/05 23:50:22 simakov Exp $
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

/** \file
 *
 *  \brief Daubechies 9/7 wavelet transform (Lifting)
 *
 *  This file contains lifting implementation of a famous Daubechies 9/7
 *  wavelet transform. Lifting transforms are faster than generic
 *  filter-based counterparts, but they lack uniformity.
 *
 *  \section References
 *
 *  <a href="http://qccpack.sourceforge.net/">QccPack, James E. Fowler</a> */

#ifndef __DAUB97LIFT_H__
#define __DAUB97LIFT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup daub97lift Daubechies 9/7 wavelet transform (Lifting) */
/*@{*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <common.h>

/** ALPHA coefficient */
#define ALPHA     -1.58615986717275
/** BETA coefficient */
#define BETA      -0.05297864003258
/** GAMMA coefficient */
#define GAMMA      0.88293362717904
/** DELTA coefficient */
#define DELTA      0.44350482244527
/** EPSILON coefficient */
#define EPSILON    1.14960430535816

/** One dimensional Daubechies 9/7 wavelet decomposition
 *
 *  This function performes one stage of 1D wavelet decomposition
 *  of \a signal_in using Daubechies 9/7 lifting transform. The result is
 *  stored in \a signal_out. On return, the first half of \a signal_out
 *  will be occupied with lowpass coefficients, the second half - with highpass
 *  coefficients.
 *
 *  \param signal_in Input signal
 *  \param signal_out Output signal
 *  \param signal_length Signal length
 *
 *  \return \c VOID
 *
 *  \note \a signal_length should be even. */
inline local void daub97lift_analysis_1D_even(coeff_t *signal_in,
                                              coeff_t *signal_out,
                                              int signal_length);

/** One dimensional wavelet reconstruction
 *
 *  This function performes one stage of 1D wavelet reconstruction
 *  of \a signal_in using Daubechies 9/7 lifting transform. The result is
 *  stored in \a signal_out.
 *
 *  \param signal_in Input signal
 *  \param signal_out Output signal
 *  \param signal_length Signal length
 *
 *  \return \c VOID
 *
 *  \note \a signal_length should be even. */
inline local void daub97lift_synthesis_1D_even(coeff_t *signal_in,
                                               coeff_t *signal_out,
                                               int signal_length);

/** One dimensional Daubechies 9/7 wavelet decomposition
 *
 *  This function performes one stage of 1D wavelet decomposition
 *  of \a signal_in using Daubechies 9/7 lifting transform. The result is
 *  stored in \a signal_out. On return, the first half of \a signal_out
 *  will be occupied with lowpass coefficients, the second half - with highpass
 *  coefficients.
 *
 *  \param signal_in Input signal
 *  \param signal_out Output signal
 *  \param signal_length Signal length
 *
 *  \return \c VOID
 *
 *  \note \a signal_length should be odd, as a consequence
 *  there will be one extra lowpass coefficient. */
inline local void daub97lift_analysis_1D_odd(coeff_t *signal_in,
                                             coeff_t *signal_out,
                                             int signal_length);

/** One dimensional wavelet reconstruction
 *
 *  This function performes one stage of 1D wavelet reconstruction
 *  of \a signal_in using Daubechies 9/7 lifting transform. The result is
 *  stored in \a signal_out.
 *
 *  \param signal_in Input signal
 *  \param signal_out Output signal
 *  \param signal_length Signal length
 *
 *  \return \c VOID
 *
 *  \note \a signal_length should be odd. */
inline local void daub97lift_synthesis_1D_odd(coeff_t *signal_in,
                                              coeff_t *signal_out,
                                              int signal_length);

/* Those functions are placed here in order to be inline-ed */

inline local void daub97lift_analysis_1D_even(coeff_t *signal_in,
                                              coeff_t *signal_out,
                                              int signal_length)
{
    int i;

    for (i = 1; i < signal_length - 2; i += 2) {
        signal_in[i] += ALPHA * (signal_in[i - 1] + signal_in[i + 1]);
    }

    signal_in[signal_length - 1] += 2 * ALPHA * signal_in[signal_length - 2];
    signal_in[0] += 2 * BETA * signal_in[1];

    for (i = 2; i < signal_length; i += 2) {
        signal_in[i] += BETA * (signal_in[i + 1] + signal_in[i - 1]);
    }

    for (i = 1; i < signal_length - 2; i += 2) {
        signal_in[i] += GAMMA * (signal_in[i - 1] + signal_in[i + 1]);
    }

    signal_in[signal_length - 1] += 2 * GAMMA * signal_in[signal_length - 2];
    signal_in[0] = EPSILON * (signal_in[0] + 2 * DELTA * signal_in[1]);

    for (i = 2; i < signal_length; i += 2) {
        signal_in[i] = EPSILON * (signal_in[i] + DELTA * (signal_in[i + 1] +
            signal_in[i - 1]));
    }

    for (i = 1; i < signal_length; i += 2) {
        signal_in[i] /= (-EPSILON);
    }

    {
        int half = signal_length / 2;
        coeff_t *even = signal_out;
        coeff_t *odd = signal_out + half;

        for (i = 0; i < half; i++) {
            even[i] = signal_in[i * 2];
            odd[i] = signal_in[i * 2 + 1];
        }
    }
}

inline local void daub97lift_synthesis_1D_even(coeff_t *signal_in,
                                               coeff_t *signal_out,
                                               int signal_length)
{
    int i;

    {
        int half = signal_length / 2;
        coeff_t *even = signal_in;
        coeff_t *odd = signal_in + half;

        for (i = 0; i < half; i++) {
            signal_out[i * 2] = even[i];
            signal_out[i * 2 + 1] = odd[i];
        }
    }

    for (i = 1; i < signal_length; i += 2) {
        signal_out[i] *= (-EPSILON);
    }

    signal_out[0] = signal_out[0] / EPSILON - 2 * DELTA * signal_out[1];

    for (i = 2; i < signal_length; i += 2) {
        signal_out[i] = signal_out[i] / EPSILON - DELTA * (signal_out[i + 1] +
            signal_out[i - 1]);
    }

    for (i = 1; i < signal_length - 2; i += 2) {
        signal_out[i] -= GAMMA * (signal_out[i - 1] + signal_out[i + 1]);
    }

    signal_out[signal_length - 1] -= 2 * GAMMA * signal_out[signal_length - 2];
    signal_out[0] -= 2 * BETA * signal_out[1];

    for (i = 2; i < signal_length; i += 2) {
        signal_out[i] -= BETA * (signal_out[i + 1] + signal_out[i - 1]);
    }

    for (i = 1; i < signal_length - 2; i += 2) {
        signal_out[i] -= ALPHA * (signal_out[i - 1] + signal_out[i + 1]);
    }

    signal_out[signal_length - 1] -= 2 * ALPHA * signal_out[signal_length - 2];
}

inline local void daub97lift_analysis_1D_odd(coeff_t *signal_in,
                                             coeff_t *signal_out,
                                             int signal_length)
{
    int i;

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_in[i] += ALPHA * (signal_in[i - 1] + signal_in[i + 1]);
    }

    signal_in[0] += 2 * BETA * signal_in[1];

    for (i = 2; i < signal_length - 2; i += 2) {
        signal_in[i] += BETA * (signal_in[i + 1] + signal_in[i - 1]);
    }

    signal_in[signal_length - 1] += 2 * BETA * signal_in[signal_length - 2];

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_in[i] += GAMMA * (signal_in[i - 1] + signal_in[i + 1]);
    }

    signal_in[0] = EPSILON * (signal_in[0] + 2 * DELTA * signal_in[1]);

    for (i = 2; i < signal_length - 2; i += 2) {
        signal_in[i] = EPSILON * (signal_in[i] + DELTA * (signal_in[i + 1] +
            signal_in[i - 1]));
    }

    signal_in[signal_length - 1] = EPSILON * (signal_in[signal_length - 1] +
        2 * DELTA * signal_in[signal_length - 2]);

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_in[i] /= (-EPSILON);
    }

    {
        int half = signal_length / 2 + 1;
        coeff_t *even = signal_out;
        coeff_t *odd = signal_out + half;

        for (i = 0; i < half - 1; i++) {
            even[i] = signal_in[i * 2];
            odd[i] = signal_in[i * 2 + 1];
        }

        even[half - 1] = signal_in[signal_length - 1];
    }
}

inline local void daub97lift_synthesis_1D_odd(coeff_t *signal_in,
                                              coeff_t *signal_out,
                                              int signal_length)
{
    int i;

    {
        int half = signal_length / 2 + 1;
        coeff_t *even = signal_in;
        coeff_t *odd = signal_in + half;

        for (i = 0; i < half - 1; i++) {
            signal_out[i * 2] = even[i];
            signal_out[i * 2 + 1] = odd[i];
        }

        signal_out[signal_length - 1] = even[half - 1];
    }

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_out[i] *= (-EPSILON);
    }

    signal_out[0] = signal_out[0] / EPSILON - 2 * DELTA * signal_out[1];

    for (i = 2; i < signal_length - 2; i += 2) {
        signal_out[i] = signal_out[i] / EPSILON - DELTA * (signal_out[i + 1] +
            signal_out[i - 1]);
    }

    signal_out[signal_length - 1] = signal_out[signal_length - 1] / EPSILON -
        2 * DELTA * signal_out[signal_length - 2];

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_out[i] -= GAMMA * (signal_out[i - 1] + signal_out[i + 1]);
    }

    signal_out[0] -= 2 * BETA * signal_out[1];

    for (i = 2; i < signal_length - 2; i += 2) {
        signal_out[i] -= BETA * (signal_out[i + 1] + signal_out[i - 1]);
    }

    signal_out[signal_length - 1] -= 2 * BETA * signal_out[signal_length - 2];

    for (i = 1; i < signal_length - 1; i += 2) {
        signal_out[i] -= ALPHA * (signal_out[i - 1] + signal_out[i + 1]);
    }
}

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __DAUB97LIFT_H__ */
