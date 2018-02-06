/*
 * $Id: color.h,v 1.18 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Color space convertion
 *
 *  This file contains routines for color space conversion.
 *
 *  \section References
 *
 *  International Telecommunications Union, ITU-R BT.601 */

#ifndef __COLOR_H__
#define __COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup color Color space conversion */
/*@{*/

#include <common.h>

/** Round value to the nearest integer */
#define ROUND(_x)               ((_x) < 0 ? (int) ((_x) - 0.5) : (int) ((_x) + 0.5))
/** Enclose value in the [0..255] interval */
#define CLIP(_x)                ((_x) < 0 ? 0 : ((_x) > 255 ? 255 : ROUND((_x))))

/** RGB to YCbCr conversion
 *
 *  This function converts image from RGB to YCbCr color space.
 *
 *  \param R Red channel
 *  \param G Green channel
 *  \param B Blue channel
 *  \param Y Luma channel
 *  \param Cb Chroma-blue channel
 *  \param Cr Chroma-red channel
 *  \param width Image width
 *  \param height Image height
 *
 *  \return \c VOID */
void convert_RGB_to_YCbCr(coeff_t **R, coeff_t **G, coeff_t **B,
                          coeff_t **Y, coeff_t **Cb, coeff_t **Cr,
                          int width, int height);

/** YCbCr to RGB conversion
 *
 *  This function converts image from YCbCr to RGB color space.
 *
 *  \param Y Luma channel
 *  \param Cb Chroma-blue channel
 *  \param Cr Chroma-red channel
 *  \param R Red channel
 *  \param G Green channel
 *  \param B Blue channel
 *  \param width Image width
 *  \param height Image height
 *
 *  \return \c VOID
 *
 *  \note On return, all values are enclosed within [0..255] interval. */
void convert_YCbCr_to_RGB(coeff_t **Y, coeff_t **Cb, coeff_t **Cr,
                          coeff_t **R, coeff_t **G, coeff_t **B,
                          int width, int height);

/** Channel clipping
 *
 *  This function encloses (clips) each \a channel value within [0..255] interval
 *  with appropriative rounding.
 *
 *  \param channel Channel to clip
 *  \param width Image width
 *  \param height Image height
 *
 *  \return \c VOID */
void clip_channel(coeff_t **channel, int width, int height);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __COLOR_H__ */
