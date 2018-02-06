/*
 * $Id: pad.h,v 1.17 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Padding
 *
 *  The library expects that the image consists of square blocks
 *  of certain size. Namely, 2^N or 2^N + 1 depending on selected
 *  wavelet transform mode. Nevertheless, most real-life images
 *  do not meet this strict requirement. That`s why we need to pad
 *  boundary image blocks to the full size. Missing data is obtained
 *  using pixel mirroring. */

#ifndef __PAD_H__
#define __PAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup pad Padding */
/*@{*/

#include <common.h>

/** Channel extension
 *
 *  This function extends \a input_channel using mirroring
 *  operation. The result is stored in the \a output_channel.
 *  Note that the \a output_channel must be greater than or
 *  equal to the \a input_channel in both width and height.
 *  Minimal channel size allowed is 1x1 pixels. Boundary
 *  pixels are duplicated.
 *
 *  \param input_channel Input channel
 *  \param output_channel Output channel
 *  \param input_width Input channel width
 *  \param input_height Input channel height
 *  \param output_width Output channel width
 *  \param output_height Output channel height
 *
 *  \return \c VOID */
void extend_channel(unsigned char **input_channel,
                    coeff_t **output_channel,
                    int input_width, int input_height,
                    int output_width, int output_height);

/** Channel extraction
 *
 *  This function extracts a block of pixels from the
 *  \a input_channel and stores it in the \a output_channel.
 *  Note that the \a output_channel size must be less than
 *  or equal to the \a input_channel in both width and height.
 *  Minimal channel size allowed is 1x1 pixels.
 *
 *  \param input_channel Input channel
 *  \param output_channel Output channel
 *  \param input_width Input channel width
 *  \param input_height Input channel height
 *  \param output_width Output channel width
 *  \param output_height Output channel height
 *
 *  \return \c VOID */
void extract_channel(coeff_t **input_channel,
                     unsigned char **output_channel,
                     int input_width, int input_height,
                     int output_width, int output_height);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __PAD_H__ */
