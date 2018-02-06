/*
 * $Id: merge_split.h,v 1.17 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief Merge and split
 *
 *  The library expects that color image consists of 3 independent
 *  channels: Y, Cb and Cr. In order to provide smooth image scaling
 *  corresponding bitstreams should be merged into a singe bitstream.
 *  On decoding, the inverse operation should be taken. */

#ifndef __MERGE_SPLIT_H__
#define __MERGE_SPLIT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup merge_split Merge and split */
/*@{*/

#include <common.h>

/** Merge two channels
 *
 *  This function merges \a channel_A with \a channel_B. Result is stored
 *  in the \a channel_AB. Note that on return, combined \a channel_AB can
 *  be further merged with another channel.
 *
 *  \param channel_A Channel A
 *  \param channel_B Channel B
 *  \param channel_AB Combined channel AB
 *  \param len_A Length of the \a channel_A
 *  \param len_B Length of the \a channel_B
 *
 *  \note The caller should allocate at least \a len_A + \a len_B
 *  bytes for the \a channel_AB array.
 *
 *  \return \c VOID */
void merge_channels(unsigned char *channel_A, unsigned char *channel_B,
                    unsigned char *channel_AB, int len_A, int len_B);

/** Split combined channel
 *
 *  This function splits combined \a channel_AB into
 *  the \a channel_A and \a channel_B. It is important to note
 *  that \a channel_AB can be truncated at any position beforehand.
 *  In this case \a channel_A and \a channel_B will contain only
 *  part of original data. Real amount of saved bytes will be stored
 *  in the \a real_len_A and \a real_len_B respectively.
 *
 *  \param channel_AB Combined channel AB
 *  \param channel_A Channel A
 *  \param channel_B Channel B
 *  \param len_AB Current length of the \a channel_AB
 *  \param len_A Original length of the \a channel_A
 *  \param len_B Original length of the \a channel_B
 *  \param real_len_A Number of bytes actually used in the \a channel_A
 *  \param real_len_B Number of bytes actually used in the \a channel_B
 *
 *  \note The caller should allocate at least \a len_AB bytes for
 *  \a channel_A and \a channel_B arrays.
 *
 *  \return \c VOID */
void split_channels(unsigned char *channel_AB,
                    unsigned char *channel_A, unsigned char *channel_B,
                    int len_AB, int len_A, int len_B,
                    int *real_len_A, int *real_len_B);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __MERGE_SPLIT_H__ */
