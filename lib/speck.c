/*
 * $Id: speck.c,v 1.62 2010/04/05 05:01:04 simakov Exp $
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

#include <common.h>
#include <speck.h>
#include <mem_alloc.h>
#include <list.h>
#include <bit_io.h>
#include <filter.h>
#include <color.h>

/* Before you dive into the sources, note that
 * X and Y axes here are swapped. In other words,
 * X denotes vertical position while Y denotes
 * horizontal position. The origin is the top-left
 * point. Now you a warned. */

local int max_coeff(int **channel, int channel_size)
{
    int i, j, max = 0;

    for (i = 0; i < channel_size; i++) {
        for (j = 0; j < channel_size; j++) {
            if (ABS(channel[i][j]) > max) {
                max = ABS(channel[i][j]);
            }
        }
    }

    return max;
}

local int validate_set(pixel_set *set, int channel_size)
{
    int base_size = channel_size & (~1);

    /* Basic checks */
    if ((set->type != TYPE_POINT) && (set->type != TYPE_S) && (set->type != TYPE_I))
        return 0;

    if ((set->x < 0) || (set->x >= channel_size))
        return 0;

    if ((set->y < 0) || (set->y >= channel_size))
        return 0;

    if ((set->width <= 0) || (set->width > channel_size))
        return 0;

    if ((set->height <= 0) || (set->height > channel_size))
        return 0;

    if (set->x + set->height > channel_size)
        return 0;

    if (set->y + set->width > channel_size)
        return 0;

    switch (set->type) {
        case TYPE_POINT:
        {
            /* Single point */
            if ((set->width != 1) || (set->height != 1))
                return 0;

            return 1;
        }
        case TYPE_S:
        {
            /* Set of type 'S' */
            int scale;

            if ((set->width == 1) && (set->height == 1))
                return 0;

            if (channel_size == base_size) {
                /* Normal mode */
                if (set->width != set->height)
                    return 0;

                if (!is_power_of_two(set->width))
                    return 0;

                scale = number_of_bits(set->width) - 1;

                if ((set->x) & (~(~0 << scale)))
                    return 0;

                if ((set->y) & (~(~0 << scale)))
                    return 0;

                return 1;
            } else {
                /* OTLPF mode */
                if ((set->x == 0) && (set->y == 0)) {
                    /* Origin */
                    if (set->width != set->height)
                        return 0;

                    if (!is_power_of_two(set->width - 1))
                        return 0;

                    return 1;
                } else if ((set->x == 0) && (set->y > 1)) {
                    /* Top border */
                    if (set->height - set->width != 1)
                        return 0;

                    if (!is_power_of_two(set->width))
                        return 0;

                    scale = number_of_bits(set->width) - 1;

                    if ((set->y - 1) & (~(~0 << scale)))
                        return 0;

                    return 1;
                } else if ((set->x > 1) && (set->y == 0)) {
                    /* Left border */
                    if (set->width - set->height != 1)
                        return 0;

                    if (!is_power_of_two(set->height))
                        return 0;

                    scale = number_of_bits(set->height) - 1;

                    if ((set->x - 1) & (~(~0 << scale)))
                        return 0;

                    return 1;
                } else if ((set->x > 1) && (set->y > 1)) {
                    /* Internal */
                    if (set->width != set->height)
                        return 0;

                    if (!is_power_of_two(set->width))
                        return 0;

                    scale = number_of_bits(set->width) - 1;

                    if ((set->x - 1) & (~(~0 << scale)))
                        return 0;

                    if ((set->y - 1) & (~(~0 << scale)))
                        return 0;

                    return 1;
                } else {
                    return 0;
                }
            }
        }
        case TYPE_I:
        {
            /* Set of type 'I' */
            if (set->x != set->y)
                return 0;

            if (set->width != set->height)
                return 0;

            if (set->x + set->width != channel_size)
                return 0;

            if (channel_size == base_size) {
                /* Normal mode */
                if (!is_power_of_two(set->x))
                    return 0;
            } else {
                /* OTLPF mode */
                if (!is_power_of_two(set->x - 1))
                    return 0;
            }

            return 1;
        }
        default:
        {
            return 0;
        }
    }
}

local int significance_test(pixel_set *set, int threshold,
                            int **channel, int channel_size)
{
#ifdef ENABLE_SET_VALIDATION
    /* Ensure that the set is valid */
    assert(validate_set(set, channel_size));
#endif

    assert(threshold > 0);

    switch (set->type) {
        case TYPE_POINT:
        {
            /* Single point */
            return (ABS(channel[set->x][set->y]) >= threshold);
            break;
        }
        case TYPE_S:
        {
            /* Set of type 'S' */
            int x, y;

            for (x = set->x; x < set->x + set->height; x++) {
                for (y = set->y; y < set->y + set->width; y++) {
                    if (ABS(channel[x][y]) >= threshold) {
                        return 1;
                    }
                }
            }

            return 0;
            break;
        }
        case TYPE_I:
        {
            /* Set of type 'I' */
            int x, y;

            for (x = 0; x < channel_size; x++) {
                for (y = 0; y < channel_size; y++) {
                    if ((x >= set->x) || (y >= set->y)) {
                        if (ABS(channel[x][y]) >= threshold) {
                            return 1;
                        }
                    }
                }
            }

            return 0;
            break;
        }
        default:
        {
            assert(0);
            break;
        }
    }

    /* This code is unreachable, just to be on a safe side */
    assert(0);

    /* Fake return disables 'not all control paths return a value' warning */
    return -1;
}

local void select_part_type(pixel_set *set)
{
    /* Guess the set type after split operation */
    if ((set->width == 1) && (set->height == 1)) {
        set->type = TYPE_POINT;
    } else if ((set->width == 0) || (set->height == 0)) {
        set->type = TYPE_EMPTY;
    } else {
        set->type = TYPE_S;
    }
}

local void split_set(pixel_set *set, pixel_set *part1, pixel_set *part2,
                     pixel_set *part3, pixel_set *part4, int channel_size)
{
    int base_size = channel_size & (~1);

#ifdef ENABLE_SET_VALIDATION
    /* Ensure that the set is valid */
    assert(validate_set(set, channel_size));
#endif

    switch (set->type) {
        case TYPE_S:
        {
            /* Split parent set of type 'S' */
            part1->x = set->x;
            part1->y = set->y;
            part1->width = (set->width + 1) / 2;
            part1->height = (set->height + 1) / 2;
            select_part_type(part1);

            part2->x = set->x;
            part2->y = set->y + (set->width + 1) / 2;
            part2->width = set->width / 2;
            part2->height = (set->height + 1) / 2;
            select_part_type(part2);

            part3->x = set->x + (set->height + 1) / 2;
            part3->y = set->y;
            part3->width = (set->width + 1) / 2;
            part3->height = set->height / 2;
            select_part_type(part3);

            part4->x = set->x + (set->height + 1) / 2;
            part4->y = set->y + (set->width + 1) / 2;
            part4->width = set->width / 2;
            part4->height = set->height / 2;
            select_part_type(part4);

            break;
        }
        case TYPE_I:
        {
            /* Split parent set of type 'I' */
            int p0, p1;
            int scale;

            scale = number_of_bits(set->x - (channel_size != base_size));

            p0 = set->x;
            p1 = (1 << scale) + (channel_size != base_size);

            part1->x = part1->y = p1;
            part1->width = part1->height = channel_size - p1;
            part1->type = (p1 == channel_size) ? TYPE_EMPTY : TYPE_I;

            part2->x = 0;
            part2->y = p0;
            part2->width = p1 - p0;
            part2->height = p0;
            select_part_type(part2);

            part3->x = p0;
            part3->y = 0;
            part3->width = p0;
            part3->height = p1 - p0;
            select_part_type(part3);

            part4->x = part4->y = p0;
            part4->width = part4->height = p1 - p0;
            select_part_type(part4);

            break;
        }
        default:
        {
            assert(0);
            break;
        }
    }
}

local linked_list **alloc_LIS_slots(int channel_size)
{
    linked_list **LIS_slots;
    int n_slots;
    int i;

    /* Think of this structure as a list of lists. Splitting
     * entire list into several slots speed-ups algorithm:
     * one slot for each scale. */
    n_slots = number_of_bits(channel_size);
    LIS_slots = (linked_list **) xmalloc(n_slots * sizeof(linked_list *));

    for (i = 0; i < n_slots; i++) {
        LIS_slots[i] = alloc_linked_list();
    }

    return LIS_slots;
}

local void free_LIS_slots(linked_list **LIS_slots, int channel_size)
{
    int n_slots;
    int i;

    n_slots = number_of_bits(channel_size);

    for (i = 0; i < n_slots; i++) {
        free_linked_list(LIS_slots[i]);
    }

    free(LIS_slots);
}

local void assign_set(list_node *node, pixel_set *set)
{
    PIXEL_SET(node)->type = set->type;
    PIXEL_SET(node)->x = set->x;
    PIXEL_SET(node)->y = set->y;
    PIXEL_SET(node)->width = set->width;
    PIXEL_SET(node)->height = set->height;
}

local void zero_channel(int **channel, int channel_size)
{
    int i, j;

    /* Reset everything to zero */
    for (i = 0; i < channel_size; i++) {
        for (j = 0; j < channel_size; j++) {
            channel[i][j] = 0;
        }
    }
}

local int speck_encode_S(int **channel, int channel_size,
                         pixel_set *set, linked_list **LIS_slots,
                         linked_list *LSP, bit_buffer *bb,
                         int threshold)
{
    pixel_set new_sets[4];
    int result;
    int st[4];
    int flag;
    int i;

    /* Split parent set */
    split_set(set, &new_sets[0], &new_sets[1], &new_sets[2], &new_sets[3], channel_size);

    /* Test each set for significance skipping over empty sets */
    for (flag = 0, i = 3; i >= 0; i--) {
        if (new_sets[i].type == TYPE_EMPTY) {
            continue;
        }

        st[i] = significance_test(&new_sets[i], threshold, channel, channel_size);

        if (i) {
            flag |= st[i];
        }

        /* If parent set is significant, but first three
         * child sets are not, than undoubtedly fourth
         * child set is significant: there is no need
         * to code this explicitly. Using this trick
         * saves some bit-budget. */
        if (i || flag) {
            result = st[i] ? write_1(bb) : write_0(bb);
            RETURN_IF_OVERFLOW(result);
        }
    }

    /* Process non-empty sets using their significance information */
    for (i = 0; i < 4; i++) {
        if (new_sets[i].type == TYPE_EMPTY) {
            continue;
        }

        if (st[i]) {
            /* Significant set */
            if (new_sets[i].type == TYPE_POINT) {
                /* Single point */
                list_node *new_node;

                /* Encode coefficient sign */
                result = channel[new_sets[i].x][new_sets[i].y] > 0 ? write_0(bb) : write_1(bb);
                RETURN_IF_OVERFLOW(result);

                new_node = alloc_list_node(sizeof(pixel_set));
                assign_set(new_node, &new_sets[i]);
                append_list_node(LSP, new_node);
            } else {
                /* Encode set of type 'S' */
                result = speck_encode_S(channel, channel_size, &new_sets[i],
                                        LIS_slots, LSP, bb, threshold);
    
                RETURN_IF_OVERFLOW(result);
            }
        } else {
            /* Insignificant set */
            list_node *new_node = alloc_list_node(sizeof(pixel_set));
            assign_set(new_node, &new_sets[i]);
            prepend_list_node(LIS_slots[SLOT_INDEX((&new_sets[i]))], new_node);            
        }
    }

    return BIT_BUFFER_OK;
}

local int speck_process_S(int **channel, int channel_size,
                          list_node *node, linked_list *slot,
                          linked_list **LIS_slots, linked_list *LSP,
                          bit_buffer *bb, int threshold,
                          int coding_stage)
{
    pixel_set *set;
    int result;
    int st;

    /* Test the set for significance */
    set = PIXEL_SET(node);
    st = significance_test(set, threshold, channel, channel_size);

    result = st ? write_1(bb) : write_0(bb);
    RETURN_IF_OVERFLOW(result);

    if (st) {
        /* Significant set */
        if (set->type == TYPE_POINT) {
            /* Single point: encode coefficient sign */
            result = channel[set->x][set->y] > 0 ? write_0(bb) : write_1(bb);
            RETURN_IF_OVERFLOW(result);

            if (coding_stage == STAGE_S) {
                remove_list_node_link(slot, node);
            }

            append_list_node(LSP, node);
        } else {
            /* Encode set of type 'S' */
            result = speck_encode_S(channel, channel_size, set,
                                    LIS_slots, LSP, bb, threshold);

            RETURN_IF_OVERFLOW(result);

            if (coding_stage == STAGE_S) {
                remove_list_node(slot, node);
            } else {
                free_list_node(node);
            }
        }
    } else {
        /* Insignificant set */
        if (coding_stage == STAGE_I) {
            prepend_list_node(LIS_slots[SLOT_INDEX(set)], node);
        }
    }

    return BIT_BUFFER_OK;
}

local int speck_encode_I(int **channel, int channel_size, pixel_set *I,
                         linked_list **LIS_slots, linked_list *LSP,
                         bit_buffer *bb, int threshold)
{
    pixel_set new_sets[3];
    int result;
    int i;

    /* Split parent set */
    split_set(I, I, &new_sets[0], &new_sets[1], &new_sets[2], channel_size);

    /* Process child sets of type 'S' */
    for (i = 0; i < 3; i++) {
        list_node *node = alloc_list_node(sizeof(pixel_set));
        assign_set(node, &new_sets[i]);

        /* Process child set of type 'S' */
        result = speck_process_S(channel, channel_size, node,
                                 NULL, LIS_slots, LSP, bb,
                                 threshold, STAGE_I);

        if (result == BIT_BUFFER_OVERFLOW) {
            free_list_node(node);
            return result;
        }
    }

    /* Process child set of type 'I' */
    result = speck_process_I(channel, channel_size, I,
                             LIS_slots, LSP, bb, threshold);

    return result;
}

local int speck_process_I(int **channel, int channel_size, pixel_set *I,
                          linked_list **LIS_slots, linked_list *LSP,
                          bit_buffer *bb, int threshold)
{
    int result;
    int st;

    /* Skip over empty sets */
    if (I->type == TYPE_EMPTY) {
        return BIT_BUFFER_OK;
    }

    /* Test the set for significance */
    st = significance_test(I, threshold, channel, channel_size);

    result = st ? write_1(bb) : write_0(bb);
    RETURN_IF_OVERFLOW(result);

    if (st) {
        /* Encode set of type 'I' */
        result = speck_encode_I(channel, channel_size, I,
                                LIS_slots, LSP, bb, threshold);

        RETURN_IF_OVERFLOW(result);
    }

    return BIT_BUFFER_OK;
}

local int encode_sorting_pass(int **channel, int channel_size,
                              linked_list **LIS_slots, linked_list *LSP,
                              pixel_set *I, bit_buffer *bb,
                              int threshold)
{
    int n_slots;
    int result;
    int i;

    n_slots = number_of_bits(channel_size);

    /* Travels through all LIS slots */
    for (i = 0; i < n_slots; i++) {
        linked_list *cur_slot = LIS_slots[i];
        list_node *cur_node;

        /* Skip over empty slots */
        CONTINUE_IF_EMPTY(cur_slot);

        /* Get first slot node */
        cur_node = cur_slot->first;

        /* Process all nodes within this slot */
        while (cur_node) {
            list_node *next_node = cur_node->next;

            /* Process set of type 'S' */
            result = speck_process_S(channel, channel_size, cur_node,
                                     cur_slot, LIS_slots, LSP, bb,
                                     threshold, STAGE_S);

            RETURN_IF_OVERFLOW(result);

            /* Next node */
            cur_node = next_node;
        }
    }

    /* Process set of type 'I' */
    result = speck_process_I(channel, channel_size, I,
                             LIS_slots, LSP, bb, threshold);

    return result;
}

local int encode_refinement_pass(int **channel, linked_list *LSP,
                                 bit_buffer *bb, int threshold)
{
    list_node *node;
    int result;

    node = LSP->first;
    threshold <<= 1;

    /* Travels through all nodes in LSP */
    while (node) {
        pixel_set *set = PIXEL_SET(node);
        int coeff = ABS(channel[set->x][set->y]);

        /* Output next bit */
        if (coeff >= threshold) {
            result = coeff & (threshold >> 1) ? write_1(bb) : write_0(bb);
            RETURN_IF_OVERFLOW(result);
        }

        /* Next node */
        node = node->next;
    }

    return BIT_BUFFER_OK;
}

local int speck_decode_S(int **channel, int channel_size,
                         pixel_set *set, linked_list **LIS_slots,
                         linked_list *LSP, bit_buffer *bb,
                         int threshold)
{
    pixel_set new_sets[4];
    int result;
    int st[4];
    int flag;
    int i;

    /* Split parent set */
    split_set(set, &new_sets[0], &new_sets[1], &new_sets[2], &new_sets[3], channel_size);

    /* Test each set for significance skipping over empty sets */
    for (flag = 0, i = 3; i >= 0; i--) {
        if (new_sets[i].type == TYPE_EMPTY) {
            continue;
        }

        if (i) {
            result = read_bit(bb, &st[i]);
            RETURN_IF_UNDERFLOW(result);

            flag |= st[i];
        } else {
            if (flag) {
                result = read_bit(bb, &st[i]);
                RETURN_IF_UNDERFLOW(result);
            } else {
                /* Implicitly significant set */
                st[i] = 1;
            }
        }
    }

    /* Process non-empty sets using their significance information */
    for (i = 0; i < 4; i++) {
        if (new_sets[i].type == TYPE_EMPTY) {
            continue;
        }

        if (st[i]) {
            /* Significant set */
            if (new_sets[i].type == TYPE_POINT) {
                /* Single point */
                list_node *new_node;
                int sign = 0;

                result = read_bit(bb, &sign);
                RETURN_IF_UNDERFLOW(result);

                /* Decode coefficient sign */
                if (sign) {
                    channel[new_sets[i].x][new_sets[i].y] =
                        -(threshold + (threshold >> 1));
                } else {
                    channel[new_sets[i].x][new_sets[i].y] =
                         (threshold + (threshold >> 1));
                }

                new_node = alloc_list_node(sizeof(pixel_set));
                assign_set(new_node, &new_sets[i]);
                append_list_node(LSP, new_node);
            } else {
                /* Decode set of type 'S' */
                result = speck_decode_S(channel, channel_size, &new_sets[i],
                                        LIS_slots, LSP, bb, threshold);
    
                RETURN_IF_UNDERFLOW(result);
            }
        } else {
            /* Insignificant set */
            list_node *new_node = alloc_list_node(sizeof(pixel_set));
            assign_set(new_node, &new_sets[i]);
            prepend_list_node(LIS_slots[SLOT_INDEX((&new_sets[i]))], new_node);            
        }
    }

    return BIT_BUFFER_OK;
}

local int speck_unprocess_S(int **channel, int channel_size,
                            list_node *node, linked_list *slot,
                            linked_list **LIS_slots, linked_list *LSP,
                            bit_buffer *bb, int threshold,
                            int coding_stage)
{
    pixel_set *set;
    int result;
    int st;

    set = PIXEL_SET(node);

    /* Read set significance information */
    result = read_bit(bb, &st);
    RETURN_IF_UNDERFLOW(result);

    if (st) {
        /* Significant set */
        if (set->type == TYPE_POINT) {
            int sign = 0;

            /* Single point: read coefficient sign */
            result = read_bit(bb, &sign);
            RETURN_IF_UNDERFLOW(result);

            if (sign) {
                channel[set->x][set->y] = -(threshold + (threshold >> 1));
            } else {
                channel[set->x][set->y] =  (threshold + (threshold >> 1));
            }

            if (coding_stage == STAGE_S) {
                remove_list_node_link(slot, node);
            }

            append_list_node(LSP, node);
        } else {
            /* Decode set of type 'S' */
            result = speck_decode_S(channel, channel_size, set,
                                    LIS_slots, LSP, bb, threshold);

            RETURN_IF_UNDERFLOW(result);

            if (coding_stage == STAGE_S) {
                remove_list_node(slot, node);
            } else {
                free_list_node(node);
            }
        }
    } else {
        /* Insignificant set */
        if (coding_stage == STAGE_I) {
            prepend_list_node(LIS_slots[SLOT_INDEX(set)], node);
        }
    }

    return BIT_BUFFER_OK;
}

local int speck_decode_I(int **channel, int channel_size, pixel_set *I,
                         linked_list **LIS_slots, linked_list *LSP,
                         bit_buffer *bb, int threshold)
{
    pixel_set new_sets[3];
    int result;
    int i;

    /* Split parent set */
    split_set(I, I, &new_sets[0], &new_sets[1], &new_sets[2], channel_size);

    /* Unprocess sets of type 'S' */
    for (i = 0; i < 3; i++) {
        list_node *node = alloc_list_node(sizeof(pixel_set));
        assign_set(node, &new_sets[i]);

        result = speck_unprocess_S(channel, channel_size, node,
                                   NULL, LIS_slots, LSP, bb,
                                   threshold, STAGE_I);

        if (result == BIT_BUFFER_UNDERFLOW) {
            free_list_node(node);
            return result;
        }
    }

    /* Unprocess set of type 'I' */
    result = speck_unprocess_I(channel, channel_size, I,
                               LIS_slots, LSP, bb, threshold);

    return result;
}

local int speck_unprocess_I(int **channel, int channel_size,
                            pixel_set *I, linked_list **LIS_slots,
                            linked_list *LSP, bit_buffer *bb,
                            int threshold)
{
    int result;
    int st;

    /* Skip over empty sets */
    if (I->type == TYPE_EMPTY) {
        return BIT_BUFFER_OK;
    }

    /* Read significance information */
    result = read_bit(bb, &st);
    RETURN_IF_UNDERFLOW(result);

    if (st) {
        result = speck_decode_I(channel, channel_size, I,
                                LIS_slots, LSP, bb, threshold);

        RETURN_IF_UNDERFLOW(result);
    }

    return BIT_BUFFER_OK;
}

local int decode_sorting_pass(int **channel, int channel_size,
                              linked_list **LIS_slots, linked_list *LSP,
                              pixel_set *I, bit_buffer *bb,
                              int threshold)
{
    int n_slots;
    int result;
    int i;

    n_slots = number_of_bits(channel_size);

    /* Travels through all LIS slots */
    for (i = 0; i < n_slots; i++) {
        linked_list *cur_slot = LIS_slots[i];
        list_node *cur_node;

        /* Skip over empty slots */
        CONTINUE_IF_EMPTY(cur_slot);

        /* Get first node */
        cur_node = cur_slot->first;

        /* Process all nodes within this slot */
        while (cur_node) {
            list_node *next_node = cur_node->next;

            /* Unprocess set of type 'S' */
            result = speck_unprocess_S(channel, channel_size, cur_node,
                                       cur_slot, LIS_slots, LSP,
                                       bb, threshold, STAGE_S);

            RETURN_IF_UNDERFLOW(result);

            /* Next node */
            cur_node = next_node;
        }
    }

    /* Unprocess set of type 'I' */
    result = speck_unprocess_I(channel, channel_size, I,
                               LIS_slots, LSP, bb, threshold);

    return result;
}

local int decode_refinement_pass(int **channel, linked_list *LSP,
                                 bit_buffer *bb, int threshold)
{
    list_node *node;
    int result;
    int mask;

    node = LSP->first;

    mask = threshold;
    threshold <<= 1;

    /* Travels through all nodes in LSP */
    while (node) {
        pixel_set *set = PIXEL_SET(node);

        int coeff = ABS(channel[set->x][set->y]);
        int sign = channel[set->x][set->y] < 0;

        if (coeff >= threshold) {
            int bit = 0;

            /* Read and shift-in next bit */
            result = read_bit(bb, &bit);
            RETURN_IF_OVERFLOW(result);

            if (bit) {
                coeff |= mask;
            } else {
                coeff &= ~mask;
            }

            coeff |= (mask >> 1);
            channel[set->x][set->y] = sign ? -coeff : coeff;
        }

        /* Next node */
        node = node->next;
    }

    return BIT_BUFFER_OK;
}

local void speck_init(linked_list **LIS_slots, pixel_set *I,
                      int channel_size, int mode)
{
    list_node *root;

    root = alloc_list_node(sizeof(pixel_set));

    /* Setup root node */
    if (mode == MODE_NORMAL) {
        PIXEL_SET(root)->type = TYPE_POINT;
        PIXEL_SET(root)->x = PIXEL_SET(root)->y = 0;
        PIXEL_SET(root)->width = PIXEL_SET(root)->height = 1;

        I->type = TYPE_I;
        I->x = I->y = 1;
        I->width = I->height = channel_size - 1;

        prepend_list_node(LIS_slots[0], root);
    } else {
        PIXEL_SET(root)->type = TYPE_S;
        PIXEL_SET(root)->x = PIXEL_SET(root)->y = 0;
        PIXEL_SET(root)->width = PIXEL_SET(root)->height = 2;

        I->type = TYPE_I;
        I->x = I->y = 2;
        I->width = I->height = channel_size - 2;

        prepend_list_node(LIS_slots[1], root);
    }
}

int speck_encode(int **channel, int channel_size,
                 unsigned char *buf, int buf_size)
{
    int threshold_bits;
    int threshold;
    int result;
    int mode;
    int n_bytes;

    linked_list **LIS_slots;
    linked_list *LSP;
    pixel_set *I;

    bit_buffer *bb;

    mode = channel_size & 1;

    /* Sanity checks */
    assert(buf_size >= MIN_SPECK_BUF_SIZE);
    assert(channel_size >= 2);

    /* Allocate list of significant pixels (LSP),
     * list of lists of insignificant sets (LIS_slots),
     * and set of type 'I' */
    LSP = alloc_linked_list();
    LIS_slots = alloc_LIS_slots(channel_size);
    I = (pixel_set *) xmalloc(sizeof(pixel_set));

    /* Setup initial encoding threshold */
    threshold_bits = number_of_bits(max_coeff(channel, channel_size));
    threshold = threshold_bits ? (1 << (threshold_bits - 1)) : 0;

    /* Allocate bit-buffer */
    bb = (bit_buffer *) xmalloc(sizeof(bit_buffer));

    /* Initialize bit-buffer */
    init_bits(bb, buf, buf_size);
    write_bits(bb, threshold_bits, THRESHOLD_BITS);

    /* Setup encoder */
    speck_init(LIS_slots, I, channel_size, mode);

    /* Travels through all bit planes */
    while (threshold > 0) {
        /* Sorting pass */
        result = encode_sorting_pass(channel, channel_size, LIS_slots, LSP, I, bb, threshold);
        BREAK_IF_OVERFLOW(result);

        /* Refinement pass */
        result = encode_refinement_pass(channel, LSP, bb, threshold);
        BREAK_IF_OVERFLOW(result);

        /* Proceed to the next bit plane */
        threshold >>= 1;
    }

    /* Flush bit-buffer */
    flush_bits(bb);
    n_bytes = bb->next - bb->start;

    free(bb);
    free(I);
    free_LIS_slots(LIS_slots, channel_size);
    free_linked_list(LSP);

    return n_bytes;
}

void speck_decode(unsigned char *buf, int buf_size,
                  int **channel, int channel_size)
{
    int threshold_bits;
    int threshold;
    int result;
    int mode;

    linked_list **LIS_slots;
    linked_list *LSP;
    pixel_set *I;

    bit_buffer *bb;

    mode = channel_size & 1;

    /* Sanity checks */
    assert(buf_size >= MIN_SPECK_BUF_SIZE);
    assert(channel_size >= 2);

    /* Reset output channel */
    zero_channel(channel, channel_size);

    /* Allocate list of significant pixels (LSP),
     * list of lists of insignificant sets (LIS_slots),
     * and set of type 'I' */
    LSP = alloc_linked_list();
    LIS_slots = alloc_LIS_slots(channel_size);
    I = (pixel_set *) xmalloc(sizeof(pixel_set));

    /* Allocate bit-buffer */
    bb = (bit_buffer *) xmalloc(sizeof(bit_buffer));

    /* Initialize bit-buffer */
    init_bits(bb, buf, buf_size);
    read_bits(bb, &threshold_bits, THRESHOLD_BITS);

    /* Read encoding threshold */
    threshold = threshold_bits ? (1 << (threshold_bits - 1)) : 0;
    speck_init(LIS_slots, I, channel_size, mode);

    /* Travels through all bit planes */
    while (threshold > 0) {
        /* Decode sorting pass */
        result = decode_sorting_pass(channel, channel_size, LIS_slots, LSP, I, bb, threshold);
        BREAK_IF_UNDERFLOW(result);

        /* Decode refinement pass */
        result = decode_refinement_pass(channel, LSP, bb, threshold);
        BREAK_IF_UNDERFLOW(result);

        /* Proceed to the next bit plane */
        threshold >>= 1;
    }

    free(bb);
    free(I);
    free_LIS_slots(LIS_slots, channel_size);
    free_linked_list(LSP);
}
