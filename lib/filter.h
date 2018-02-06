/*
 * $Id: filter.h,v 1.22 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Signal filtering
 *
 *  Wavelet transform implementation based on filter banks.
 *
 *  \section References
 *
 *  Gilbert Strang, Truong Nguyen "Wavelets and Filter Banks".
 *
 *  Jianxin Wei, Mark Pickering, Michael Frater, John Arnold,
 *  John Boman, Wenjun Zeng "Boundary Artefact Reduction Using
 *  Odd Tile Length and the Low Pass First Convention (OTLPF)". */

#ifndef __FILTER_H__
#define __FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Use __inline instead of inline under MSVC compiler */
#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline  __inline
#endif

/** \addtogroup wavelet Wavelet transform */
/*@{*/

#include <common.h>
#include <filterbank.h>

/** Normal mode
 *
 *  This mode assumes that image is square and height = width = 2 ^ N. */
#define MODE_NORMAL             0
/** OTLPF mode
 *
 *  This mode also assumes that image is square, but height = width = (2 ^ N) + 1.
 *  In a few words, OTLPF is some kind of hack to reduce boundary artefacts
 *  when image is broken into several tiles. Due to mathematical constrains
 *  this method can be applied to biorthogonal filters only. For more
 *  information see references. */
#define MODE_OTLPF              1

/** Periodic signal extension
 *
 *  This function extends signal in a periodic fashion.
 *  For example: ... 2 3 4 | 1 2 3 4 | 1 2 3 ...
 *  This kind of extension is used with orthogonal filters.
 *
 *  \param index Sample index
 *  \param length Signal length
 *
 *  \return Real sample index within array bounds
 *
 *  \note Actually, signal is not extended as the function name states.
 *  This function just computes real sample index within array bounds. */
inline local int periodic_extension(int index, int length);

/** Symmetric-whole signal extension
 *
 *  This function extends signal in symmetric-whole fasion.
 *  For example: ... 4 3 2 | 1 2 3 4 | 3 2 1 ... This kind
 *  of extension is used with biorthogonal filters of odd length.
 *
 *  \param index Sample index
 *  \param length Signal length
 *
 *  \return Real sample index within array bounds
 *
 *  \note Actually, signal is not extended as the function name states.
 *  This function just computes real sample index within array bounds. */
inline local int symmetric_W_extension(int index, int length);

/** Symmetric-half signal extension
 *
 *  This function extends signal in symmetric-half fasion.
 *  For example: ... 3 2 1 | 1 2 3 4 | 4 3 2 ... (i.e. boundary
 *  samples are duplicated). This kind of extension is used with
 *  biorthogonal filters of even length.
 *
 *  \param index Sample index
 *  \param length Signal length
 *
 *  \return Real sample index within array bounds
 *
 *  \note Actually, signal is not extended as the function name states.
 *  This function just computes real sample index within array bounds. */
inline local int symmetric_H_extension(int index, int length);

/** Signal downsampling
 *
 *  This function downsamples signal by the factor of two. Depending
 *  on \a phase, #PHASE_EVEN or #PHASE_ODD, odd-numbered (1, 3, 5, ...)
 *  or even-numbered (0, 2, 4, ...) samples are rejected respectively.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param input_length Input signal length
 *  \param output_length Output signal length
 *  \param phase Downsampling phase
 *
 *  \return \c VOID
 *
 *  \note Caller must allocate enough space for \a output_signal beforehand. */
inline local void downsample_signal(coeff_t *input_signal, coeff_t *output_signal,
                                    int input_length, int output_length, int phase);

/** Signal upsampling
 *
 *  This function is inverse to the previous one. It inserts zeros between
 *  \a input_signal samples. Depending on \a phase, #PHASE_EVEN or #PHASE_ODD,
 *  zeros are insered into the odd-numbered (1, 3, 5, ...) or
 *  even-numbered (0, 2, 4, ...) positions respectively.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param input_length Input signal length
 *  \param output_length Output signal length
 *  \param phase Upsampling phase
 *
 *  \return \c VOID
 *
 *  \note Caller must allocate enough space for \a output_signal beforehand. */
inline local void upsample_signal(coeff_t *input_signal, coeff_t *output_signal,
                                  int input_length, int output_length, int phase);

/** Periodic signal filtering
 *
 *  This function filters \a input_signal of length \a signal_length
 *  into the \a output_signal using specified \a filter. Boundary
 *  samples are evaluated using peridic extension.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param signal_length Signal length
 *  \param filter Filter
 *
 *  \return \c VOID
 *
 *  \note \a filter must be orthogonal.
 *  \note \a signal_length must be even. */
inline local void filter_periodic(coeff_t *input_signal, coeff_t *output_signal,
                                  int signal_length, filter_t *filter);

/** Symmetric signal filtering
 *
 *  This function filters \a input_signal of length \a signal_length
 *  into the \a output_signal using specified \a filter. Boundary
 *  samples are evaluated using symmetric extension.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param signal_length Signal length
 *  \param filter Filter
 *
 *  \return \c VOID
 *
 *  \note \a filter must be biorthogonal.
 *  \note \a signal_length can be either even or odd.
 *
 *  \todo Add support for even-length biorthogonal filters. */
inline local void filter_symmetric(coeff_t *input_signal, coeff_t *output_signal,
                                   int signal_length, filter_t *filter);

/** One dimensional wavelet decomposition
 *
 *  This function performes one stage of 1D wavelet decomposition
 *  of \a input_signal using filter bank \a fb. The result is
 *  stored in \a output_signal. This operation requires one temporary
 *  array of length \a signal_length. On return, the first half of \a output_signal
 *  will be occupied with lowpass coefficients, the second half - with highpass
 *  coefficients.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param temp Temporary array
 *  \param signal_length Signal length
 *  \param fb Filter bank
 *
 *  \return \c VOID
 *
 *  \note If \a signal_length is odd and \a fb is biorthogonal, then
 *  there will be one extra lowpass coefficient. */
local void analysis_1D(coeff_t *input_signal, coeff_t *output_signal,
                       coeff_t *temp, int signal_length, filterbank_t *fb);

/** One dimensional wavelet reconstruction
 *
 *  This function performes one stage of 1D wavelet reconstruction
 *  of \a input_signal using filter bank \a fb. The result is
 *  stored in \a output_signal. This operation requires tree temporary
 *  arrays of length \a signal_length.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param temp1 Temporary array 1
 *  \param temp2 Temporary array 2
 *  \param temp3 Temporary array 3
 *  \param signal_length Signal length
 *  \param fb Filter bank
 *
 *  \return \c VOID */
local void synthesis_1D(coeff_t *input_signal, coeff_t *output_signal,
                        coeff_t *temp1, coeff_t *temp2, coeff_t *temp3,
                        int signal_length, filterbank_t *fb);

/** Two dimensional wavelet decomposition
 *
 *  This function performes N stages of 2D wavelet decomposition of
 *  \a input_signal using filter bank \a fb. Image is assumed to be square:
 *  if \a mode = #MODE_NORMAL, then width = height = signal_length = 2 ^ N;
 *  if \a mode = #MODE_OTLPF, then width = height = signal_length = (2 ^ N) + 1.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param signal_length Signal length (width = height)
 *  \param mode Either #MODE_NORMAL or #MODE_OTLPF
 *  \param fb Filter bank
 *
 *  \return \c VOID */
void analysis_2D(coeff_t **input_signal, coeff_t **output_signal,
                 int signal_length, int mode, filterbank_t *fb);

/** Two dimensional wavelet reconstruction
 *
 *  This function performes N stages of 2D wavelet reconstruction of
 *  \a input_signal using filter bank \a fb. Image is assumed to be square:
 *  if \a mode = #MODE_NORMAL, then width = height = signal_length = 2 ^ N;
 *  if \a mode = #MODE_OTLPF, then width = height = signal_length = (2 ^ N) + 1.
 *
 *  \param input_signal Input signal
 *  \param output_signal Output signal
 *  \param signal_length Signal length (width = height)
 *  \param mode Either #MODE_NORMAL or #MODE_OTLPF
 *  \param fb Filter bank
 *
 *  \return \c VOID */
void synthesis_2D(coeff_t **input_signal, coeff_t **output_signal,
                  int signal_length, int mode, filterbank_t *fb);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_H__ */
