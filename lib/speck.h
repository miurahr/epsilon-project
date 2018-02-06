/*
 * $Id: speck.h,v 1.50 2010/02/05 23:50:22 simakov Exp $
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
 *  \brief SPECK wavelet codec
 *
 *  This file represents SPECK - The Set-Partitioning Embedded Block
 *  wavelet codec. To understand the algorithm you have to read
 *  original article from William Pearlman and Asad Islam. Also it
 *  is highly recommended to get familar with coding example.
 *
 *  \warning The Set-Partitioning Embedded Block (SPECK) algorithm is
 *  protected by US Patent #6,671,413 and also may be patented in your
 *  country.
 *
 *  \section References
 *
 *  <a href="http://www.cipr.rpi.edu/~pearlman/">William A. Pearlman home page</a>
 *
 *  <a href="http://www.cipr.rpi.edu/~pearlman/papers/vcip99_ip.pdf">
 *  A. Islam and W. A. Pearlman, An Embedded and Efficient Low-Complexity
 *  Hierarchical Image Coder, Visual Communications and Image Processing 99,
 *  Proceedings of SPIE Vol. 3653, pp. 294-305, Jan. 1999.</a>
 *
 *  <a href="http://www.cipr.rpi.edu/~pearlman/papers/speck_example.pdf">
 *  SPECK coding example</a> */

#ifndef __SPECK_H__
#define __SPECK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup speck SPECK wavelet codec */
/*@{*/

#include <common.h>
#include <list.h>
#include <bit_io.h>

/** Pixel set of type 'point' */
#define TYPE_POINT              0
/** Pixel set of type 'S' */
#define TYPE_S                  1
/** Pixel set of type 'I' */
#define TYPE_I                  2
/** Empty pixel set */
#define TYPE_EMPTY              3

/** Processing sets of type 'S' */
#define STAGE_S                 0
/** Processing sets of type 'I' */
#define STAGE_I                 1

/** Minimal SPECK buffer size */
#define MIN_SPECK_BUF_SIZE      1
/** Reserve 6 bits for \a theshold_bits parameter */
#define THRESHOLD_BITS          6

/** Cast data pointer as \ref pixel_set structure */
#define PIXEL_SET(_set)         ((pixel_set *) (_set->data))
/** Select inserting index for array of LIS slots */
#define SLOT_INDEX(_set)        (number_of_bits(MIN(_set->width, _set->height)) - 1)

/** Break if buffer is full */
#define BREAK_IF_OVERFLOW(_x)   if (_x == BIT_BUFFER_OVERFLOW) break
/** Return if buffer is full */
#define RETURN_IF_OVERFLOW(_x)  if (_x == BIT_BUFFER_OVERFLOW) return _x
/** Break if buffer is empty */
#define BREAK_IF_UNDERFLOW(_x)  if (_x == BIT_BUFFER_UNDERFLOW) break
/** Return if buffer is empty */
#define RETURN_IF_UNDERFLOW(_x) if (_x == BIT_BUFFER_UNDERFLOW) return _x
/** Contunue if list is empty */
#define CONTINUE_IF_EMPTY(_x)   if (LIST_IS_EMPTY(_x)) continue

/** This structure represents pixel_set */
typedef struct pixel_set_tag {
    /** Set type */
    short type;
    /** X coordinate */
    short x;
    /** Y coordinate */
    short y;
    /** Set width */
    short width;
    /** Set height */
    short height;
} pixel_set;

/** Find maximal coefficient
 *
 *  This function returns absolute value of maximal
 *  wavelet coefficient.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *
 *  \return Maximal coefficient value */
local int max_coeff(int **channel, int channel_size);

/** Validate set
 *
 *  The process of splitting, moving and testing sets in the SPECK
 *  algorithm is a bit tricky. This function can be thought as a very
 *  strict validation tool.
 *
 *  \param set Set to validate
 *  \param channel_size Channel size
 *
 *  \return \c 1 for valid sets and \c 0 for invalid ones */
local int validate_set(pixel_set *set, int channel_size);

/** Significance test
 *
 *  The purpose of this function is to compare \a set against \a threshold.
 *  If the \a set have a coefficient greater than or equal to \a threshold,
 *  then it is said that the \a set is significant. If all coefficients
 *  of the \a set are below \a threshold, then it is said that the \a set
 *  is insignificant.
 *
 *  \param set Set to test
 *  \param threshold Threshold to compare against
 *  \param channel Channel
 *  \param channel_size Channel size
 *
 *  \return \c 1 for significant sets and \c 0 for insignificant ones */
local int significance_test(pixel_set *set, int threshold,
                            int **channel, int channel_size);

/** Select partition type
 *
 *  This function selects type of the \a set.
 *
 *  \param set Set
 *
 *  \return \c VOID */
local void select_part_type(pixel_set *set);

/** Split set
 *
 *  This function splits the \a set into pieces. Actual
 *  splitting rule is defined within SPECK algorithm.
 *
 *  \param set Set to split
 *  \param part1 First part
 *  \param part2 Second part
 *  \param part3 Third part
 *  \param part4 Fourth part
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
local void split_set(pixel_set *set, pixel_set *part1, pixel_set *part2,
                     pixel_set *part3, pixel_set *part4, int channel_size);

/** Allocate array of LIS slots
 *
 *  This function allocates array of LIS slots.
 *
 *  \param channel_size Channel size
 *
 *  \return Pointer to newly allocated structure */
local linked_list **alloc_LIS_slots(int channel_size);

/** Release array of LIS slots
 *
 *  This function releases array of LIS slots.
 *
 *  \param LIS_slots Array of LIS slots
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
local void free_LIS_slots(linked_list **LIS_slots, int channel_size);

/** Assign set attributes
 *
 *  This function assigns \a set attributes to the \a node.
 *
 *  \param node Destination node
 *  \param set Source set
 *
 *  \return \c VOID */
local void assign_set(list_node *node, pixel_set *set);

/** Reset channel
 *
 *  This function resets all \a channel components to zero.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
local void zero_channel(int **channel, int channel_size);

/** Encode set of type 'S'
 *
 *  This function encodes \a set of type 'S'.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param set Set to encode
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int speck_encode_S(int **channel, int channel_size,
                         pixel_set *set, linked_list **LIS_slots,
                         linked_list *LSP, bit_buffer *bb,
                         int threshold);

/** Process set of type 'S'
 *
 *  This function extracts \ref pixel_set structure from the \a node,
 *  and encodes it using \ref speck_encode_S function.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param node Current node
 *  \param slot Current LIS slot
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *  \param coding_stage Either \ref STAGE_S or \ref STAGE_I
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int speck_process_S(int **channel, int channel_size, list_node *node,
                          linked_list *slot, linked_list **LIS_slots,
                          linked_list *LSP, bit_buffer *bb,
                          int threshold, int coding_stage);

/** Encode set of type 'I'
 *
 *  This function encodes set of type 'I'.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param I Set of type I
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int speck_encode_I(int **channel, int channel_size, pixel_set *I,
                         linked_list **LIS_slots, linked_list *LSP,
                         bit_buffer *bb, int threshold);

/** Process set of type 'I'
 *
 *  This function encodes set \a I using \ref speck_encode_I function.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param I Set of type I
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int speck_process_I(int **channel, int channel_size, pixel_set *I,
                          linked_list **LIS_slots, linked_list *LSP,
                          bit_buffer *bb, int threshold);

/** Encode sorting pass
 *
 *  The SPECK encoding algorithm alternates two types of passes
 *  through the data: sorting pass and refinement pass. This
 *  function implements the first one.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param I Set of type I
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int encode_sorting_pass(int **channel, int channel_size,
                              linked_list **LIS_slots, linked_list *LSP,
                              pixel_set *I, bit_buffer *bb, int threshold);

/** Encode refinement pass
 *
 *  The SPECK encoding algorithm alternates two types of passes
 *  through the data: sorting pass and refinement pass. This
 *  function implements the second one.
 *
 *  \param channel Channel
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_OVERFLOW */
local int encode_refinement_pass(int **channel, linked_list *LSP,
                                 bit_buffer *bb, int threshold);

/** Decode set of type 'S'
 *
 *  This function is inverse to \ref speck_encode_S.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param set Set to decode
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int speck_decode_S(int **channel, int channel_size,
                         pixel_set *set, linked_list **LIS_slots,
                         linked_list *LSP, bit_buffer *bb,
                         int threshold);

/** Unprocess set of type 'S'
 *
 *  This function is inverse to \ref speck_process_S.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param node Current node
 *  \param slot Current LIS slot
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *  \param coding_stage Either \ref STAGE_S or \ref STAGE_I
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int speck_unprocess_S(int **channel, int channel_size,
                            list_node *node, linked_list *slot,
                            linked_list **LIS_slots,
                            linked_list *LSP, bit_buffer *bb,
                            int threshold, int coding_stage);

/** Decode set of type 'I'
 *
 *  This function is inverse to \ref speck_encode_I.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param I Set of type I
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int speck_decode_I(int **channel, int channel_size,
                         pixel_set *I, linked_list **LIS_slots,
                         linked_list *LSP, bit_buffer *bb,
                         int threshold);

/** Unprocess set of type 'I'
 *
 *  This function is inverse to \ref speck_process_I.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param I Set of type I
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int speck_unprocess_I(int **channel, int channel_size,
                            pixel_set *I, linked_list **LIS_slots,
                            linked_list *LSP, bit_buffer *bb,
                            int threshold);

/** Decode sorting pass
 *
 *  The SPECK decoding algorithm alternates two types of passes
 *  through the data: sorting pass and refinement pass. This
 *  function implements the first one.
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param LIS_slots Array of LIS slots
 *  \param LSP List of Significant Pixels
 *  \param I Set of type I
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int decode_sorting_pass(int **channel, int channel_size,
                              linked_list **LIS_slots,
                              linked_list *LSP, pixel_set *I,
                              bit_buffer *bb, int threshold);

/** Decode refinement pass
 *
 *  The SPECK decoding algorithm alternates two types of passes
 *  through the data: sorting pass and refinement pass. This
 *  function implements the second one.
 *
 *  \param channel Channel
 *  \param LSP List of Significant Pixels
 *  \param bb Bit-buffer
 *  \param threshold Threshold
 *
 *  \return Either \ref BIT_BUFFER_OK or \ref BIT_BUFFER_UNDERFLOW */
local int decode_refinement_pass(int **channel, linked_list *LSP,
                                 bit_buffer *bb, int threshold);

/** Initialize SPECK encoder or decoder
 *
 *  This function initializes SPECK encoder or decoder.
 *
 *  \param LIS_slots Array of LIS slots
 *  \param I Set of type I
 *  \param channel_size Channel size
 *  \param mode Either \ref MODE_NORMAL or \ref MODE_OTLPF
 *
 *  \return \c VOID */
local void speck_init(linked_list **LIS_slots, pixel_set *I,
                      int channel_size, int mode);

/** Encode channel using SPECK algorithm
 *
 *  This function encodes \a channel of size \a channel_size
 *  into the buffer \a buf of size \a buf_size.
 *
 *  \note Depending on encoding mode, minimal channel
 *  size is \c 2 (for \ref MODE_NORMAL) or \c 3
 *  (for \ref MODE_OTLPF).
 *
 *  \note Minimal buffer size is \ref MIN_SPECK_BUF_SIZE
 *
 *  \param channel Channel
 *  \param channel_size Channel size
 *  \param buf Buffer
 *  \param buf_size Buffer size
 *
 *  \return Number of bytes in \a buf actualy used by encoder */
int speck_encode(int **channel, int channel_size,
                 unsigned char *buf, int buf_size);

/** Decode channel using SPECK algorithm
 *
 *  This function decodes \a channel of size \a channel_size
 *  from the buffer \a buf of size \a buf_size.
 *
 *  \note Depending on encoding mode, minimal channel
 *  size is \c 2 (for \ref MODE_NORMAL) or \c 3
 *  (for \ref MODE_OTLPF).
 *
 *  \note Minimal buffer size is \ref MIN_SPECK_BUF_SIZE
 *
 *  \param buf Buffer
 *  \param buf_size Buffer size
 *  \param channel Channel
 *  \param channel_size Channel size
 *
 *  \return \c VOID */
void speck_decode(unsigned char *buf, int buf_size,
                  int **channel, int channel_size);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __SPECK_H__ */
