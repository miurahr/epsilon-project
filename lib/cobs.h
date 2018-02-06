/*
 * $Id: cobs.h,v 1.16 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Byte stuffing
 *
 *  This file contains routines for efficient byte stuffing.
 *  The algorithm used here called COBS, which is stands for Consistent
 *  Overhead Byte Stuffing. Byte stuffing is very useful for parallel
 *  image processing and improves overall system robustness.
 *
 *  \section References
 *
 *  Stuart Cheshire and Mary Baker, "Consistent Overhead Byte Stuffing".
 *  IEEE/ACM Transactions on Networking. vol. 7 pp. 159-172 April 1999. */

#ifndef __COBS_H__
#define __COBS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup stuffing Byte stuffing */
/*@{*/

#include <common.h>

/** Byte stuffing
 *
 *  This function performes byte stuffing. All zero-valued
 *  bytes in the \a input_data will be eliminated. The result
 *  will be stored in the \a output_data. This operation involves
 *  some marginal data expansion, no more than 0.4% at worst case.
 *
 *  \param input_data Input data
 *  \param output_data Output data
 *  \param input_length Input data length
 *  \param output_length Output data length
 *
 *  \return Number of bytes actually used in the \a output_data
 *
 *  \note Caller must allocate enough space for the \a output_data
 *  beforehand. This value can be calculated by the following
 *  formula: \a input_length + (\a input_length / 254) + 1. */
int stuff_data(unsigned char *input_data, unsigned char *output_data,
               int input_length, int output_length);

/** Byte unstuffing
 *
 *  This function is inverse to the previous one. It recovers original data.
 *  The result will be stored in the \a output_data.
 *
 *  \param input_data Input data
 *  \param output_data Output data
 *  \param input_length Input data length
 *  \param output_length Output data length
 *
 *  \return Number of bytes actually used in the \a output_data
 *
 *  \note Caller must allocate enough space for the \a output_data
 *  beforehand. Safe value is \a input_length.  */
int unstuff_data(unsigned char *input_data, unsigned char *output_data,
                 int input_length, int output_length);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __COBS_H__ */
