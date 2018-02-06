/*
 * $Id: libmain.h,v 1.19 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Auxiliary support routines
 *
 *  This file contains auxiliary support routines for
 *  the library. While being top-level they are not
 *  intended for direct user access. */

#ifndef __LIBMAIN_H__
#define __LIBMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup misc Miscellanea */
/*@{*/

#include <common.h>
#include <filterbank.h>
#include <filter.h>

/** Round a channel
 *
 *  This function rounds each \a in_channel element to the
 *  nearest integer and stores it in the \a out_channel.
 *
 *  \param in_channel Input channel
 *  \param out_channel Output channel
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
local void round_channel(coeff_t **in_channel, int **out_channel,
                         int channel_size);

/** Copy a channel
 *
 *  This function copies \a in_channel into the \a out_channel.
 *
 *  \param in_channel Input channel
 *  \param out_channel Output channel
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
local void copy_channel(int **in_channel, coeff_t **out_channel,
                        int channel_size);

/** Reset RGB channels
 *
 *  This function initializes arrays \a block_R, \a block_G
 *  and \a block_B with zero.
 *
 *  \param block_R Red channel
 *  \param block_G Green channel
 *  \param block_B Blue channel
 *  \param width Block width
 *  \param height Block height
 *
 *  \return \c VOID */
local void reset_RGB(unsigned char **block_R, unsigned char **block_G,
                     unsigned char **block_B, int width, int height);

/** Reset Y channel
 *
 *  This function initializes array \a block_Y with zero.
 *
 *  \param block_Y Luma channel
 *  \param width Block width
 *  \param height Block height
 *
 *  \return \c VOID */
local void reset_Y(unsigned char **block_Y, int width, int height);

/** Get filterbank pointer from id
 *
 *  This function gets filterbank pointer from \a id.
 *
 *  \param id Filterbank id
 *
 *  \return Filterbank pointer or \c NULL if not found */
local filterbank_t *get_fb(char *id);

/** Compute required block size
 *
 *  This function computes block size (width=height) required
 *  for encoding and decoding process. Depending on \a mode
 *  parameter, function returns nearest power of two or
 *  power of two plus one.
 *
 *  \param w Source width
 *  \param h Source height
 *  \param mode Either \ref EPS_MODE_NORMAL or \ref EPS_MODE_OTLPF
 *  \param min Minimal block size
 *
 *  \return Block size (width = height) */
local int get_block_size(int w, int h, int mode, int min);

/** Terminate block header
 *
 *  This function replaces \a n_fields -th occurence of \c ;
 *  symbol with \c 0. In other words, this function zero-terminates
 *  header.
 *
 *  \param buf Data buffer
 *  \param buf_size Buffer size
 *  \param n_fields Number of header fields
 *
 *  \return Either \ref EPS_OK or \ref EPS_FORMAT_ERROR */
local int terminate_header(unsigned char *buf, int buf_size, int n_fields);

/** Unterminate header
 *
 *  This function is inverse to the previous one. It
 *  replaces first occurence of \c 0 with \c ; symbol.
 *
 *  \param buf Data buffer
 *
 *  \return \c VOID */
local void unterminate_header(unsigned char *buf);

/** Header sanity check
 *
 *  This function ensures that header contains only
 *  legal symbols.
 *
 *  \param buf Data buffer
 *
 *  \return Either \ref EPS_OK or \ref EPS_FORMAT_ERROR */
local int header_sanity_check(unsigned char *buf);

/** Read GRAYSCALE header
 *
 *  This function reads and checks block header of type
 *  \ref EPS_GRAYSCALE_BLOCK. Result is stored in the \a hdr
 *  structure.
 *
 *  \note Structure \a hdr is undefined unless function
 *  returns \ref EPS_OK.
 *
 *  \param buf Data buffer
 *  \param buf_size Buffer size
 *  \param hdr Block header
 *
 *  \return Either \ref EPS_OK or \ref EPS_PARAM_ERROR
 *  or \ref EPS_FORMAT_ERROR */
local int read_gs_header(unsigned char *buf, int buf_size,
                         eps_block_header *hdr);

/** Read TRUECOLOR header
 *
 *  This function reads and checks block header of type
 *  \ref EPS_TRUECOLOR_BLOCK. Result is stored in the \a hdr
 *  structure.
 *
 *  \note Structure \a hdr is undefined unless function
 *  returns \ref EPS_OK.
 *
 *  \param buf Data buffer
 *  \param buf_size Buffer size
 *  \param hdr Block header
 *
 *  \return Either \ref EPS_OK or \ref EPS_PARAM_ERROR
 *  or \ref EPS_FORMAT_ERROR */
local int read_tc_header(unsigned char *buf, int buf_size,
                         eps_block_header *hdr);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __LIBMAIN_H__ */
