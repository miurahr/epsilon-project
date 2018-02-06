/*
 * $Id: dc_level.h,v 1.13 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief DC level shift
 *
 *  In order to further improve codec perfomance, input signal must be
 *  centered around zero before applying wavelet transform. One can
 *  accomplish this by subtracting mean value from each image sample.
 *  This preprocessing step results in wavelet coefficient magnitude
 *  decrease, which in turn, reduces number of bits required for
 *  image encoding. After decoding, one have to perform the inverse
 *  operation, i.e. add stored mean value to each reconstructed image sample. */

#ifndef __DC_LEVEL_H__
#define __DC_LEVEL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup dc_level DC level shift */
/*@{*/

#include <common.h>

/** DC level shift
 *
 *  This function subtracts mean value from each image sample.
 *
 *  \param channel Image channel
 *  \param width Image width
 *  \param height Image height
 *
 *  \return Mean value */
coeff_t dc_level_shift(coeff_t **channel, int width, int height);

/** DC level unshift
 *
 *  This function adds stored mean value to each
 *  reconstructed image sample.
 *
 *  \param channel Image channel
 *  \param average Average (mean) value
 *  \param width Image width
 *  \param height Image height
 *
 *  \return \c VOID */
void dc_level_unshift(coeff_t **channel, coeff_t average, int width, int height);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __DC_LEVEL_H__ */
