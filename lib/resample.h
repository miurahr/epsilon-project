/*
 * $Id: resample.h,v 1.14 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Resampling
 *
 *  It is well known that human eye is less sensitive to color
 *  than intensity. It is worthwhile to utilize this fact. We can
 *  store chroma channels with lower resolution than luma. One
 *  can achieve this through channel resampling procedure.
 *  At the moment EPSILON library uses bilinear resampling
 *  algorithm.
 *
 *  \section References
 *
 *  <a href="http://en.wikipedia.org/wiki/Bilinear_interpolation">
 *  Bilinear interpolation (Wikipedia) </a>
 *
 *  <a href="http://alglib.sources.ru/interpolation/bilinearresample.php">
 *  Bilinear resampling (in Russian)</a> */

#ifndef __RESAMPLE_H__
#define __RESAMPLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup resampling Resampling */
/*@{*/

#include <common.h>

/** Bilinear channel resampling
 *
 *  This function performes bilinear channel resampling.
 *
 *  \param input_channel Input channel
 *  \param output_channel Output channel
 *  \param input_width Input channel width
 *  \param input_height Input channel height
 *  \param output_width Output channel width
 *  \param output_height Output channel height
 *
 *  \return \c VOID
 *
 *  \note Input and output dimensions must be greater than 1. */
void bilinear_resample_channel(coeff_t **input_channel, coeff_t **output_channel,
                               int input_width, int input_height,
                               int output_width, int output_height);
/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __RESAMPLE_H__ */
