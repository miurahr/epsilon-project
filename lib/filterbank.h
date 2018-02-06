/*
 * $Id: filterbank.h,v 1.17 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Filter banks
 *
 *  This file holds defines for filter and filterbank
 *  data structures.
 *
 *  \section References
 *
 *  Gilbert Strang, Truong Nguyen "Wavelets and Filter Banks". */

#ifndef __FILTERBANK_H__
#define __FILTERBANK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup wavelet Wavelet transform */
/*@{*/

#include <common.h>

/** Causal filter: h[i] = 0, i < 0 */
#define CAUSAL                  0
/** Anticausal filter: h[i] = 0, i > 0 */
#define ANTICAUSAL              1
/** Symmetric-whole filter: h[-i] = h[i] */
#define SYMMETRIC_WHOLE         2
/** Symmetric-half filter: h[-i] = h[i - 1] */
#define SYMMETRIC_HALF          3

/** Lowpass analysis filter */
#define LOWPASS_ANALYSIS        0
/** Highpass analysis filter */
#define HIGHPASS_ANALYSIS       1
/** Lowpass synthesis filter */
#define LOWPASS_SYNTHESIS       2
/** Highpass synthesis filter */
#define HIGHPASS_SYNTHESIS      3

/** Orthogonal filterbank */
#define ORTHOGONAL              0
/** Biothogonal filterbank */
#define BIORTHOGONAL            1

/** Even subsampling phase */
#define PHASE_EVEN              0
/** Odd subsampling phase */
#define PHASE_ODD               1

/** Filter structure
 *
 *  This structure represents a standalone filter.
 *
 *  \note There is no need to keep all coefficients for symmetric
 *  filters. Only half (h[i], i >= 0) of them is kept. */
typedef struct filter_t_tag {
    /** Filter length */
    int length;
    /** Filter causality */
    int causality;
    /** Filter type */
    int type;
    /** Filter coefficients */
    coeff_t *coeffs;
} filter_t;

/** Filterbank structure
 *
 *  Filter bank consists of two filter pairs. */
typedef struct filterbank_t_tag {
    /** Short filter name (for a program) */
    char *id;
    /** Long filter name (for a user) */
    char *name;
    /** Filterbank type */
    int type;
    /** Lowpass analysis filter */
    filter_t *lowpass_analysis;
    /** Highpass analysis filter */
    filter_t *highpass_analysis;
    /** Lowpass synthesis filter */
    filter_t *lowpass_synthesis;
    /** Highpass synthesis filter */
    filter_t *highpass_synthesis;
} filterbank_t;

/** External array of all available filter banks
 *
 *  This array hold pointers to all available filter banks.
 *  Last element is always \c NULL. */
extern filterbank_t *filterbanks[];

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __FILTERBANK_H__ */
