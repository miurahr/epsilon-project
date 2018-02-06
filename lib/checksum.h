/*
 * $Id: checksum.h,v 1.15 2010/02/05 23:50:22 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006-2010 Alexander Simakov, <xander@entropyware.info>
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
 *  \brief Checksum calculation
 *
 *  This file contains routines for CRC-32 and ADLER-32 checksum
 *  calculation. At the moment program uses only CRC-32 algorithm.
 *
 *  \section References
 *
 *  <a href="http://www.ross.net/crc/">A Painless Guide to CRC Error Detection Algorithms</a><br>
 *  <a href="http://en.wikipedia.org/wiki/CRC32">Wikipedia: CRC-32</a><br>
 *  <a href="http://en.wikipedia.org/wiki/Adler-32">Wikipedia: ADLER-32</a> */

#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup checksum Checksum calculation */
/*@{*/

#include <common.h>
#include <epsilon.h>

/** Compute ADLER-32 checksum
 *
 *  This function computes ADLER-32 checksum.
 *
 *  \param data Data to sum
 *  \param length Data length
 *
 *  \return Checksum */
crc32_t epsilon_adler32(unsigned char *data, int length);

/** Compute CRC-32 checksum
 *
 *  This function computes CRC-32 checksum.
 *
 *  \param data Data to sum
 *  \param length Data length
 *
 *  \return Checksum */
crc32_t epsilon_crc32(unsigned char *data, int length);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __CHECKSUM_H__ */
