/*
 * $Id: libmain.c,v 1.63 2010/04/05 05:01:04 simakov Exp $
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

#include <epsilon.h>
#include <libmain.h>
#include <common.h>
#include <filter.h>
#include <filterbank.h>
#include <cobs.h>
#include <color.h>
#include <mem_alloc.h>
#include <dc_level.h>
#include <resample.h>
#include <checksum.h>
#include <pad.h>
#include <merge_split.h>
#include <speck.h>
#include <string.h>

local void round_channel(coeff_t **in_channel, int **out_channel,
                         int channel_size)
{
    int i, j;

    for (i = 0; i < channel_size; i++) {
        for (j = 0; j < channel_size; j++) {
            out_channel[i][j] = (int) ROUND(in_channel[i][j]);
        }
    }
}

local void copy_channel(int **in_channel, coeff_t **out_channel,
                        int channel_size)
{
    int i, j;

    /* Expand data from int to coeff_t */
    for (i = 0; i < channel_size; i++) {
        for (j = 0; j < channel_size; j++) {
            out_channel[i][j] = in_channel[i][j];
        }
    }
}

local void reset_RGB(unsigned char **block_R, unsigned char **block_G,
                     unsigned char **block_B, int width, int height)
{
    int i, j;

    /* Reset everything to zero */
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            block_R[i][j] = block_G[i][j] = block_B[i][j] = 0;
        }
    }
}

local void reset_Y(unsigned char **block_Y, int width, int height)
{
    int i, j;

    /* Reset everything to zero */
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            block_Y[i][j] = 0;
        }
    }
}

local filterbank_t *get_fb(char *id)
{
    int i, n;

    /* Get the number of all available filterbanks */
    for (n = 0; filterbanks[n]; n++);

    /* Find id in the list */
    for (i = 0; i < n; i++) {
        if (strcmp(id, filterbanks[i]->id) == 0) {
            return filterbanks[i];
        }
    }

    return NULL;
}

local int get_block_size(int w, int h, int mode, int min)
{
    int max = MAX(MAX(w, h), min);
    int bits = number_of_bits(max);

    if (mode == EPS_MODE_NORMAL) {
        /* W = H = 2 ^ N */
        if (max == (1 << (bits - 1))) {
            return max;
        } else {
            return (1 << bits);
        }
    } else {
        /* W = H = 2 ^ N + 1 */
        if (max == (1 << (bits - 1)) + 1) {
            return max;
        } else if (max == (1 << (bits - 1))) {
            return max + 1;
        } else {
            return (1 << bits) + 1;
        }
    }
}

local int terminate_header(unsigned char *buf, int buf_size, int n_fields)
{
    int field, i;

    /* Report an error if data contains at least one zero byte */
    for (i = 0; i < buf_size; i++) {
        if (!buf[i]) {
            return EPS_FORMAT_ERROR;
        }
    }

    /* Find n-th occurence of ';' symbol and replace it with zero */
    for (i = 0, field = 1; i < buf_size; i++) {
        if (buf[i] == ';') {
            if (field == n_fields) {
                buf[i] = 0;
                return EPS_OK;
            }

            field++;
        }
    }

    return EPS_FORMAT_ERROR;
}

local void unterminate_header(unsigned char *buf)
{
    buf[strlen((char *) buf)] = ';';
}

local int header_sanity_check(unsigned char *buf)
{
    int i, len;

    len = strlen((char *) buf);

    /* Check header for malicious symbols */
    for (i = 0; i < len; i++) {
        if ((buf[i] >= 'A') && (buf[i] <= 'Z')) {
            continue;
        }

        if ((buf[i] >= 'a') && (buf[i] <= 'z')) {
            continue;
        }

        if ((buf[i] >= '0') && (buf[i] <= '9')) {
            continue;
        }

        if ((buf[i] == ';') || (buf[i] == '=') ||
            (buf[i] == '-') || (buf[i] == ':'))
        {
            continue;
        }

        return EPS_FORMAT_ERROR;
    }

    return EPS_OK;
}

local int read_gs_header(unsigned char *buf, int buf_size,
                         eps_block_header *hdr)
{
    filterbank_t *fb;

    char fb_id[32];

    crc32_t hdr_crc;
    crc32_t data_crc;

    int result;
    int len;
    int n;

    char *chk_pos;
    char *str;

    /* Sanity checks */
    if (!buf || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (buf_size < 1) {
        return EPS_PARAM_ERROR;
    }

    /* Terminate header for ease of processing */
    if (terminate_header(buf, buf_size, 12) != EPS_OK) {
        return EPS_FORMAT_ERROR;
    }

    /* Check for maliciuos symbols */
    if (header_sanity_check(buf) != EPS_OK) {
        unterminate_header(buf);
        return EPS_FORMAT_ERROR;
    }

    /* Handle header as a regular null-terminated string */
    str = (char *) buf;
    len = strlen(str);

    /* Mark the position of header CRC field */
    chk_pos = strstr(str, "chk=");

    /* Parse header fields */
    result = sscanf(str,
        "type=gs;W=%d;H=%d;w=%d;h=%d;x=%d;y=%d;"
        "m=%d;dc=%d;fb=%31[a-z0-9];chk=%x;crc=%x%n",
        &hdr->hdr_data.gs.W, &hdr->hdr_data.gs.H,
        &hdr->hdr_data.gs.w, &hdr->hdr_data.gs.h,
        &hdr->hdr_data.gs.x, &hdr->hdr_data.gs.y,
        &hdr->hdr_data.gs.mode, &hdr->hdr_data.gs.dc,
        fb_id, &hdr->chk, &hdr->crc, &n);

    unterminate_header(buf);

    /* Check for parsing errors (see also sscanf(3)) */
    if ((result < 11) || (n != len)) {
        return EPS_FORMAT_ERROR;
    }

    /* Compute header & data size */
    hdr->hdr_size = len + 1;
    hdr->data_size = buf_size - hdr->hdr_size;

    /* Sanity checks */
    assert(hdr->data_size >= 0);
    assert(hdr->hdr_size + hdr->data_size == buf_size);

    /* Check transform mode */
    if ((hdr->hdr_data.gs.mode != EPS_MODE_NORMAL) &&
        (hdr->hdr_data.gs.mode != EPS_MODE_OTLPF)) {
        return EPS_FORMAT_ERROR;
    }

    /* Check image (W, H) and block (w, y, w, h) parameters for consistency */
    if ((hdr->hdr_data.gs.W <= 0) || (hdr->hdr_data.gs.H <= 0)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.gs.w < 1) || (hdr->hdr_data.gs.h < 1)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.gs.w > EPS_MAX_BLOCK_SIZE + (hdr->hdr_data.gs.mode == EPS_MODE_OTLPF)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.gs.h > EPS_MAX_BLOCK_SIZE + (hdr->hdr_data.gs.mode == EPS_MODE_OTLPF)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.gs.x < 0) || (hdr->hdr_data.gs.y < 0)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.gs.x + hdr->hdr_data.gs.w > hdr->hdr_data.gs.W) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.gs.y + hdr->hdr_data.gs.h > hdr->hdr_data.gs.H) {
        return EPS_FORMAT_ERROR;
    }

    /* Check DC level */
    if ((hdr->hdr_data.gs.dc < 0) || (hdr->hdr_data.gs.dc > 255)) {
        return EPS_FORMAT_ERROR;
    }

    /* Find filterbank by id */
    fb = get_fb(fb_id);
    if (fb) {
        hdr->hdr_data.gs.fb_id = fb->id;
    } else {
        hdr->hdr_data.gs.fb_id = NULL;
    }

    /* EPS_MODE_NORMAL is the only valid choise for orthogonal filters */
    if ((fb->type == ORTHOGONAL) && (hdr->hdr_data.gs.mode != EPS_MODE_NORMAL)) {
        return EPS_FORMAT_ERROR;
    }

    assert(chk_pos);

    /* Compute header CRC and compare it against stored one */
    hdr_crc = epsilon_crc32(buf, chk_pos - (char *) buf);
    hdr_crc = (hdr_crc ^ (hdr_crc >> 16)) & 0xffff;

    if (hdr_crc == hdr->chk) {
        hdr->chk_flag = EPS_GOOD_CRC;
    } else {
        hdr->chk_flag = EPS_BAD_CRC;
    }

    /* Compute data CRC and compare it against stored one */
    data_crc = epsilon_crc32(buf + hdr->hdr_size, hdr->data_size);

    if (data_crc == hdr->crc) {
        hdr->crc_flag = EPS_GOOD_CRC;
    } else {
        hdr->crc_flag = EPS_BAD_CRC;
    }

    return EPS_OK;
}

local int read_tc_header(unsigned char *buf, int buf_size,
                         eps_block_header *hdr)
{
    filterbank_t *fb;

    char fb_id[32];

    crc32_t hdr_crc;
    crc32_t data_crc;

    int result;
    int len;
    int n;

    char *chk_pos;
    char *str;

    /* Sanity checks */
    if (!buf || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (buf_size < 1) {
        return EPS_PARAM_ERROR;
    }

    /* Terminate header for ease of processing */
    if (terminate_header(buf, buf_size, 14) != EPS_OK) {
        return EPS_FORMAT_ERROR;
    }

    /* Check for maliciuos symbols */
    if (header_sanity_check(buf) != EPS_OK) {
        unterminate_header(buf);
        return EPS_FORMAT_ERROR;
    }

    /* Handle header as a regular null-terminated string */
    str = (char *) buf;
    len = strlen(str);

    /* Mark the position of header CRC field */
    chk_pos = strstr(str, "chk=");

    /* Parse header fields */
    result = sscanf(str,
        "type=tc;W=%d;H=%d;w=%d;h=%d;x=%d;y=%d;m=%d;"
        "r=%d;dc=%d:%d:%d;rt=%d:%d:%d;fb=%31[a-z0-9];"
        "chk=%x;crc=%x%n",
        &hdr->hdr_data.tc.W, &hdr->hdr_data.tc.H,
        &hdr->hdr_data.tc.w, &hdr->hdr_data.tc.h,
        &hdr->hdr_data.tc.x, &hdr->hdr_data.tc.y,
        &hdr->hdr_data.tc.mode, &hdr->hdr_data.tc.resample,
        &hdr->hdr_data.tc.dc_Y, &hdr->hdr_data.tc.dc_Cb,
        &hdr->hdr_data.tc.dc_Cr, &hdr->hdr_data.tc.Y_rt,
        &hdr->hdr_data.tc.Cb_rt, &hdr->hdr_data.tc.Cr_rt,
        fb_id, &hdr->chk, &hdr->crc, &n);

    unterminate_header(buf);

    /* Check for parsing errors (see also sscanf(3)) */
    if ((result < 17) || (n != len)) {
        return EPS_FORMAT_ERROR;
    }

    /* Compute header & data size */
    hdr->hdr_size = len + 1;
    hdr->data_size = buf_size - hdr->hdr_size;

    /* Sanity checks */
    assert(hdr->data_size >= 0);
    assert(hdr->hdr_size + hdr->data_size == buf_size);

    /* Check transform mode */
    if ((hdr->hdr_data.tc.mode != EPS_MODE_NORMAL) &&
        (hdr->hdr_data.tc.mode != EPS_MODE_OTLPF))
    {
        return EPS_FORMAT_ERROR;
    }

    /* Check image (W, H) and block (w, y, w, h) parameters for consistency */
    if ((hdr->hdr_data.tc.W <= 0) || (hdr->hdr_data.tc.H <= 0)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.tc.w < 1) || (hdr->hdr_data.tc.h < 1)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.tc.w > EPS_MAX_BLOCK_SIZE + (hdr->hdr_data.tc.mode == EPS_MODE_OTLPF)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.tc.h > EPS_MAX_BLOCK_SIZE + (hdr->hdr_data.tc.mode == EPS_MODE_OTLPF)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.tc.x < 0) || (hdr->hdr_data.tc.y < 0)) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.tc.x + hdr->hdr_data.tc.w > hdr->hdr_data.tc.W) {
        return EPS_FORMAT_ERROR;
    }

    if (hdr->hdr_data.tc.y + hdr->hdr_data.tc.h > hdr->hdr_data.tc.H) {
        return EPS_FORMAT_ERROR;
    }

    /* Check resampling mode */
    if ((hdr->hdr_data.tc.resample != EPS_RESAMPLE_444) &&
        (hdr->hdr_data.tc.resample != EPS_RESAMPLE_420))
    {
        return EPS_FORMAT_ERROR;
    }

    /* Check DC level for Y, Cb and Cr channels */
    if ((hdr->hdr_data.tc.dc_Y < 0) || (hdr->hdr_data.tc.dc_Y > 255)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.tc.dc_Cb < 0) || (hdr->hdr_data.tc.dc_Cb > 255)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.tc.dc_Cr < 0) || (hdr->hdr_data.tc.dc_Cr > 255)) {
        return EPS_FORMAT_ERROR;
    }

    if ((hdr->hdr_data.tc.Y_rt <= 0)  ||
        (hdr->hdr_data.tc.Cb_rt <= 0) ||
        (hdr->hdr_data.tc.Cr_rt <= 0))
    {
        return EPS_FORMAT_ERROR;
    }

    /* Find filterbank by id */
    fb = get_fb(fb_id);
    if (fb) {
        hdr->hdr_data.tc.fb_id = fb->id;
    } else {
        hdr->hdr_data.tc.fb_id = NULL;
    }

    /* EPS_MODE_NORMAL is the only valid choise for orthogonal filters */
    if ((fb->type == ORTHOGONAL) && (hdr->hdr_data.tc.mode != EPS_MODE_NORMAL)) {
        return EPS_FORMAT_ERROR;
    }

    assert(chk_pos);

    /* Compute header CRC and compare it against stored one */
    hdr_crc = epsilon_crc32(buf, chk_pos - (char *) buf);
    hdr_crc = (hdr_crc ^ (hdr_crc >> 16)) & 0xffff;

    if (hdr_crc == hdr->chk) {
        hdr->chk_flag = EPS_GOOD_CRC;
    } else {
        hdr->chk_flag = EPS_BAD_CRC;
    }

    /* Compute data CRC and compare it against stored one */
    data_crc = epsilon_crc32(buf + hdr->hdr_size, hdr->data_size);

    if (data_crc == hdr->crc) {
        hdr->crc_flag = EPS_GOOD_CRC;
    } else {
        hdr->crc_flag = EPS_BAD_CRC;
    }

    return EPS_OK;
}

int eps_read_block_header(unsigned char *buf, int buf_size,
                          eps_block_header *hdr)
{
    char *str;

    /* Sanity checks */
    if (!buf || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (buf_size < 1) {
        return EPS_PARAM_ERROR;
    }

    /* Extract first header field: block type */
    if (terminate_header(buf, buf_size, 1) != EPS_OK) {
        return EPS_FORMAT_ERROR;
    }

    str = (char *) buf;

    /* Set block type */
    if ((strlen(str) == 7) && (strcmp(str, "type=gs") == 0)) {
        hdr->block_type = EPS_GRAYSCALE_BLOCK;
    } else if ((strlen(str) == 7) && (strcmp(str, "type=tc") == 0)) {
        hdr->block_type = EPS_TRUECOLOR_BLOCK;
    } else {
        unterminate_header(buf);
        return EPS_FORMAT_ERROR;
    }

    unterminate_header(buf);

    /* Process the block using appropriative function */
    switch (hdr->block_type) {
        case EPS_GRAYSCALE_BLOCK:
        {
            return read_gs_header(buf, buf_size, hdr);
            break;
        }

        case EPS_TRUECOLOR_BLOCK:
        {
            return read_tc_header(buf, buf_size, hdr);
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
    return EPS_FORMAT_ERROR;
}

char **eps_get_fb_info(int type)
{
    char **info;
    int i, n;

    /* Get the number of all available filterbanks */
    for (n = 0; filterbanks[n]; n++);
    info = (char **) xmalloc((n + 1) * sizeof(char *));

    /* Travels the list and populate `info' with requested information */
    for (i = 0; i < n; i++) {
        if (type == EPS_FB_ID) {
            info[i] = filterbanks[i]->id;
        } else if (type == EPS_FB_NAME) {
            info[i] = filterbanks[i]->name;
        } else if (type == EPS_FB_TYPE) {
            if (filterbanks[i]->type == BIORTHOGONAL) {
                info[i] = "biorthogonal";
            } else {
                info[i] = "orthogonal";
            }
        } else {
            info[i] = NULL;
        }
    }

    /* Terminate list with NULL pointer */
    info[n] = NULL;

    return info;
}

void eps_free_fb_info(char **info)
{
    free(info);
}

void **eps_xmalloc(int size)
{
    return xmalloc(size);
}

void **eps_malloc_2D(int width, int height, int size)
{
    return malloc_2D(width, height, size);
}

void eps_free_2D(void **ptr, int width, int height)
{
    free_2D(ptr, width, height);
}

int eps_encode_grayscale_block(unsigned char **block, int W, int H, int w, int h,
                               int x, int y, unsigned char *buf, int *buf_size,
                               char *fb_id, int mode)
{
    filterbank_t *fb;

    unsigned char *buf_next;
    int bytes_left;

    unsigned char *stuff_buf;
    int stuff_bytes;
    int stuff_max;
    int stuff_cut;

    coeff_t **pad_block;
    coeff_t **dwt_block;
    int **int_block;

    int speck_bytes;
    int block_size;
    int str_len;

    unsigned char dc_int;
    coeff_t dc;

    crc32_t hdr_crc;
    crc32_t data_crc;

    unsigned char *crc_pos;

    /* Sanity checks */
    if (!block || !buf || !buf_size || !fb_id) {
        return EPS_PARAM_ERROR;
    }

    /* Check input parameters for consistency */
    if ((mode != EPS_MODE_NORMAL) && (mode != EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if ((W <= 0) || (H <= 0)) {
        return EPS_PARAM_ERROR;
    }

    if ((w < 1) || (h < 1)) {
        return EPS_PARAM_ERROR;
    }

    if (w > EPS_MAX_BLOCK_SIZE + (mode == EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if (h > EPS_MAX_BLOCK_SIZE + (mode == EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if ((x < 0) || (y < 0)) {
        return EPS_PARAM_ERROR;
    }

    if ((x + w > W) || (y + h > H)) {
        return EPS_PARAM_ERROR;
    }

    if (*buf_size < EPS_MIN_GRAYSCALE_BUF) {
        return EPS_PARAM_ERROR;
    }

    /* Find filterbank from id */
    if (!(fb = get_fb(fb_id))) {
        return EPS_UNSUPPORTED_FB;
    }

    /* EPS_MODE_NORMAL is the only valid choise for orthogonal filters */
    if ((fb->type == ORTHOGONAL) && (mode != EPS_MODE_NORMAL)) {
        return EPS_PARAM_ERROR;
    }

    buf_next = buf;
    bytes_left = *buf_size;

    /* Compute block size */
    block_size = get_block_size(w, h, mode, 2);

    /* Extend block */
    pad_block = (coeff_t **) malloc_2D(block_size, block_size, sizeof(coeff_t));
    extend_channel(block, pad_block, w, h, block_size, block_size);

    /* DC level shift */
    dc = dc_level_shift(pad_block, block_size, block_size);
    dc_int = (unsigned char) CLIP(dc);

    /* Wavelet transform */
    dwt_block = (coeff_t **) malloc_2D(block_size, block_size, sizeof(coeff_t));
    analysis_2D(pad_block, dwt_block, block_size, mode, fb);
    free_2D((void *) pad_block, block_size, block_size);

    /* Round coefficients */
    int_block = (int **) malloc_2D(block_size, block_size, sizeof(int));
    round_channel(dwt_block, int_block, block_size);
    free_2D((void *) dwt_block, block_size, block_size);

    /* Write block header */
    str_len = snprintf((char *) buf_next, bytes_left,
        "type=gs;W=%d;H=%d;w=%d;h=%d;x=%d;y=%d;"
        "m=%d;dc=%d;fb=%s;",
        W, H, w, h, x, y, mode, dc_int, fb_id);

    assert(str_len < bytes_left);

    buf_next += str_len;
    bytes_left -= str_len;

    /* Compute and save block CRC */
    hdr_crc = epsilon_crc32(buf, str_len);
    hdr_crc = (hdr_crc ^ (hdr_crc >> 16)) & 0xffff;

    str_len = snprintf((char *) buf_next, bytes_left,
                       "chk=%04x;crc=????????;", hdr_crc);

    assert(str_len < bytes_left);

    buf_next += str_len;
    bytes_left -= str_len;

    crc_pos = buf_next - 9;

    /* Encode coefficients */
    speck_bytes = speck_encode(int_block, block_size,
                               buf_next, bytes_left);

    free_2D((void *) int_block, block_size, block_size);

    /* Byte stuffing */
    stuff_max = speck_bytes + speck_bytes / 254 + 1;
    stuff_buf = (unsigned char *) xmalloc(stuff_max);
    stuff_bytes = stuff_data(buf_next, stuff_buf, speck_bytes, stuff_max);

    assert(stuff_bytes >= speck_bytes);

    /* Cut encoded stream to fit it within available space */
    stuff_cut = MIN(bytes_left, stuff_bytes);
    memcpy(buf_next, stuff_buf, stuff_cut);

    free(stuff_buf);

    /* Compute and save data CRC */
    data_crc = epsilon_crc32(buf_next, stuff_cut);
    snprintf((char *) crc_pos, 9, "%08x", data_crc);
    crc_pos[8] = ';';

    buf_next += stuff_cut;
    bytes_left -= stuff_cut;

    /* Compute real amount of available bytes */
    *buf_size = buf_next - buf;

    return EPS_OK;
}

int eps_decode_grayscale_block(unsigned char **block, unsigned char *buf,
                               eps_block_header *hdr)
{
    filterbank_t *fb;

    unsigned char *unstuff_buf;
    int unstuff_bytes;

    int **int_block;
    coeff_t **dwt_block;
    coeff_t **pad_block;

    unsigned char dc_int;
    int block_size;

    /* Sanity checks */
    if (!block || !buf || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (hdr->data_size < 1) {
        return EPS_PARAM_ERROR;
    }

    if (!hdr->hdr_data.gs.fb_id) {
        return EPS_UNSUPPORTED_FB;
    }

    /* Reset Y channel */
    reset_Y(block, hdr->hdr_data.gs.w, hdr->hdr_data.gs.h);

    /* Find filterbank from id */
    fb = get_fb(hdr->hdr_data.gs.fb_id);
    assert(fb);

    /* Unstuff data */
    unstuff_buf = (unsigned char *) xmalloc(hdr->data_size *
        sizeof(unsigned char));

    unstuff_bytes = unstuff_data(buf + hdr->hdr_size, unstuff_buf,
        hdr->data_size, hdr->data_size);

    if (unstuff_bytes == 0) {
        unstuff_buf[0] = 0;
        unstuff_bytes = 1;
    }

    /* Compute block size */
    block_size = get_block_size(hdr->hdr_data.gs.w, hdr->hdr_data.gs.h,
        hdr->hdr_data.gs.mode, 2);

    /* Decode coefficients */
    int_block = (int **) malloc_2D(block_size, block_size, sizeof(int));
    speck_decode(unstuff_buf, unstuff_bytes, int_block, block_size);
    free(unstuff_buf);

    /* Extend values from int to coeff_t */
    dwt_block = (coeff_t **) malloc_2D(block_size, block_size, sizeof(coeff_t));
    copy_channel(int_block, dwt_block, block_size);
    free_2D((void *) int_block, block_size, block_size);

    /* Inverse wavelet transform */
    pad_block = (coeff_t **) malloc_2D(block_size, block_size, sizeof(coeff_t));
    synthesis_2D(dwt_block, pad_block, block_size, hdr->hdr_data.gs.mode, fb);
    free_2D((void *) dwt_block, block_size, block_size);

    dc_int = (unsigned char) hdr->hdr_data.gs.dc;

    /* DC level unshift */
    dc_level_unshift(pad_block, (coeff_t) dc_int,
        block_size, block_size);

    /* Extract original data */
    extract_channel(pad_block, block, block_size, block_size,
        hdr->hdr_data.gs.w, hdr->hdr_data.gs.h);

    free_2D((void *) pad_block, block_size, block_size);

    return EPS_OK;
}

int eps_encode_truecolor_block(unsigned char **block_R,
                               unsigned char **block_G,
                               unsigned char **block_B,
                               int W, int H, int w, int h,
                               int x, int y, int resample,
                               unsigned char *buf, int *buf_size,
                               int Y_rt, int Cb_rt, int Cr_rt,
                               char *fb_id, int mode)
{
    filterbank_t *fb;

    unsigned char *buf_next;
    int bytes_left;

    unsigned char *buf_Y;
    unsigned char *buf_Cb;
    unsigned char *buf_Cr;

    int buf_Y_size;
    int buf_Cb_size;
    int buf_Cr_size;

    unsigned char *buf_Cb_Cr;
    unsigned char *buf_Y_Cb_Cr;

    unsigned char *stuff_buf;

    int stuff_bytes;
    int stuff_max;
    int stuff_cut;

    coeff_t **pad_block_R;
    coeff_t **pad_block_G;
    coeff_t **pad_block_B;

    coeff_t **pad_block_Y;
    coeff_t **pad_block_Cb;
    coeff_t **pad_block_Cr;

    coeff_t **block_Y;
    coeff_t **block_Cb;
    coeff_t **block_Cr;

    coeff_t **dwt_block_Y;
    coeff_t **dwt_block_Cb;
    coeff_t **dwt_block_Cr;

    int **int_block_Y;
    int **int_block_Cb;
    int **int_block_Cr;

    int block_Y_size;
    int block_Cb_size;
    int block_Cr_size;

    int speck_bytes_Y;
    int speck_bytes_Cb;
    int speck_bytes_Cr;

    int speck_bytes;

    int full_size;
    int half_size;

    coeff_t dc_Y;
    coeff_t dc_Cb;
    coeff_t dc_Cr;

    unsigned char dc_Y_int;
    unsigned char dc_Cb_int;
    unsigned char dc_Cr_int;

    crc32_t hdr_crc;
    crc32_t data_crc;

    unsigned char *crc_pos;
    int str_len;

    /* Sanity checks */
    if (!block_R || !block_G || !block_B) {
        return EPS_PARAM_ERROR;
    }

    if (!buf || !buf_size || !fb_id) {
        return EPS_PARAM_ERROR;
    }

    /* Check input parameters for consistency */
    if ((mode != EPS_MODE_NORMAL) && (mode != EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if ((W <= 0) || (H <= 0)) {
        return EPS_PARAM_ERROR;
    }

    if ((w < 1) || (h < 1)) {
        return EPS_PARAM_ERROR;
    }

    if (w > EPS_MAX_BLOCK_SIZE + (mode == EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if (h > EPS_MAX_BLOCK_SIZE + (mode == EPS_MODE_OTLPF)) {
        return EPS_PARAM_ERROR;
    }

    if ((x < 0) || (y < 0)) {
        return EPS_PARAM_ERROR;
    }

    if ((x + w > W) || (y + h > H)) {
        return EPS_PARAM_ERROR;
    }

    if ((resample != EPS_RESAMPLE_444) && (resample != EPS_RESAMPLE_420)) {
        return EPS_PARAM_ERROR;
    }

    if (*buf_size < EPS_MIN_TRUECOLOR_BUF) {
        return EPS_PARAM_ERROR;
    }

    if ((Y_rt < EPS_MIN_RT) || (Cb_rt < EPS_MIN_RT) || (Cr_rt < EPS_MIN_RT)) {
        return EPS_PARAM_ERROR;
    }

    if (Y_rt + Cb_rt + Cr_rt != 100) {
        return EPS_PARAM_ERROR;
    }

    /* Find filterbank from id */
    if (!(fb = get_fb(fb_id))) {
        return EPS_UNSUPPORTED_FB;
    }

    /* EPS_MODE_NORMAL is the only valid choise for orthogonal filters */
    if ((fb->type == ORTHOGONAL) && (mode != EPS_MODE_NORMAL)) {
        return EPS_PARAM_ERROR;
    }

    buf_next = buf;
    bytes_left = *buf_size;

    /* Compute bid-budget for each channel */
    buf_Cr_size = MAX((bytes_left / 100) * Cr_rt, 1);
    buf_Cb_size = MAX((bytes_left / 100) * Cb_rt, 1);
    buf_Y_size = bytes_left - buf_Cb_size - buf_Cr_size;

    /* Ensure that everything is ok */
    assert((buf_Y_size > 0) && (buf_Cb_size > 0) && (buf_Cr_size > 0));
    assert(buf_Y_size + buf_Cb_size + buf_Cr_size == bytes_left);

    /* Compute block sizes for full and resampled channels */
    full_size = get_block_size(w, h, mode, 4);
    half_size = full_size / 2 + (mode == EPS_MODE_OTLPF);

    /* Allocate memory for extended R,G,B channels */
    pad_block_R = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_G = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_B = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));

    /* Extend R,G,B channels */
    extend_channel(block_R, pad_block_R, w, h, full_size, full_size);
    extend_channel(block_G, pad_block_G, w, h, full_size, full_size);
    extend_channel(block_B, pad_block_B, w, h, full_size, full_size);

    /* Allocate memory for extended Y,Cb,Cr channels */
    pad_block_Y = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_Cb = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_Cr = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));

    /* Convert from R,G,B to Y,Cb,Cr color space */
    convert_RGB_to_YCbCr(pad_block_R, pad_block_G, pad_block_B,
                         pad_block_Y, pad_block_Cb, pad_block_Cr,
                         full_size, full_size);

    /* No longer needed */
    free_2D((void *) pad_block_R, full_size, full_size);
    free_2D((void *) pad_block_G, full_size, full_size);
    free_2D((void *) pad_block_B, full_size, full_size);

    if (resample == EPS_RESAMPLE_444) {
        /* No resampling: all channels are full sized */
        block_Y_size = full_size;
        block_Cb_size = full_size;
        block_Cr_size = full_size;

        /* No changes */
        block_Y = pad_block_Y;
        block_Cb = pad_block_Cb;
        block_Cr = pad_block_Cr;
    } else {
        /* Resample image using 4:2:0 scheme */
        block_Y_size = full_size;
        block_Cb_size = half_size;
        block_Cr_size = half_size;

        /* No changes in Y channel */
        block_Y = pad_block_Y;

        /* Allocate memory for resampled Cb and Cr channels */
        block_Cb = (coeff_t **) malloc_2D(half_size, half_size,
            sizeof(coeff_t));
        block_Cr = (coeff_t **) malloc_2D(half_size, half_size,
            sizeof(coeff_t));

        /* Resample Cb channel */
        bilinear_resample_channel(pad_block_Cb, block_Cb,
                                  full_size, full_size,
                                  half_size, half_size);

        /* Resample Cr channel */
        bilinear_resample_channel(pad_block_Cr, block_Cr,
                                  full_size, full_size,
                                  half_size, half_size);

        /* No longer needed */
        free_2D((void *) pad_block_Cb, full_size, full_size);
        free_2D((void *) pad_block_Cr, full_size, full_size);
    }

    /* DC level shift */
    dc_Y = dc_level_shift(block_Y, block_Y_size, block_Y_size);
    dc_Cb = dc_level_shift(block_Cb, block_Cb_size, block_Cb_size);
    dc_Cr = dc_level_shift(block_Cr, block_Cr_size, block_Cr_size);

    /* Clip DC values */
    dc_Y_int = (unsigned char) CLIP(dc_Y);
    dc_Cb_int = (unsigned char) CLIP(dc_Cb);
    dc_Cr_int = (unsigned char) CLIP(dc_Cr);

    /* Allocate memory for wavelet coefficients */
    dwt_block_Y = (coeff_t **) malloc_2D(block_Y_size, block_Y_size,
        sizeof(coeff_t));
    dwt_block_Cb = (coeff_t **) malloc_2D(block_Cb_size, block_Cb_size,
        sizeof(coeff_t));
    dwt_block_Cr = (coeff_t **) malloc_2D(block_Cr_size, block_Cr_size,
        sizeof(coeff_t));

    /* Wavelet transform */
    analysis_2D(block_Y, dwt_block_Y, block_Y_size, mode, fb);
    analysis_2D(block_Cb, dwt_block_Cb, block_Cb_size, mode, fb);
    analysis_2D(block_Cr, dwt_block_Cr, block_Cr_size, mode, fb);

    /* No longer needed */
    free_2D((void *) block_Y, block_Y_size, block_Y_size);
    free_2D((void *) block_Cb, block_Cb_size, block_Cb_size);
    free_2D((void *) block_Cr, block_Cr_size, block_Cr_size);

    /* Allocate memory for rounded wavelet coefficients */
    int_block_Y = (int **) malloc_2D(block_Y_size, block_Y_size,
        sizeof(int));
    int_block_Cb = (int **) malloc_2D(block_Cb_size, block_Cb_size,
        sizeof(int));
    int_block_Cr = (int **) malloc_2D(block_Cr_size, block_Cr_size,
        sizeof(int));

    /* Round wavelet coefficients */
    round_channel(dwt_block_Y, int_block_Y, block_Y_size);
    round_channel(dwt_block_Cb, int_block_Cb, block_Cb_size);
    round_channel(dwt_block_Cr, int_block_Cr, block_Cr_size);

    /* No longer needed */
    free_2D((void *) dwt_block_Y, block_Y_size, block_Y_size);
    free_2D((void *) dwt_block_Cb, block_Cb_size, block_Cb_size);
    free_2D((void *) dwt_block_Cr, block_Cr_size, block_Cr_size);

    /* Allocate memory for encoded data */
    buf_Y = (unsigned char *) xmalloc(buf_Y_size *
        sizeof(unsigned char));
    buf_Cb = (unsigned char *) xmalloc(buf_Cb_size *
        sizeof(unsigned char));
    buf_Cr = (unsigned char *) xmalloc(buf_Cr_size *
        sizeof(unsigned char));

    /* Encode Y,Cb,Cr channels */
    speck_bytes_Y = speck_encode(int_block_Y, block_Y_size,
                                 buf_Y, buf_Y_size);

    speck_bytes_Cb = speck_encode(int_block_Cb, block_Cb_size,
                                  buf_Cb, buf_Cb_size);


    speck_bytes_Cr = speck_encode(int_block_Cr, block_Cr_size,
                                  buf_Cr, buf_Cr_size);

    /* No longer needed */
    free_2D((void *) int_block_Y, block_Y_size, block_Y_size);
    free_2D((void *) int_block_Cb, block_Cb_size, block_Cb_size);
    free_2D((void *) int_block_Cr, block_Cr_size, block_Cr_size);

    /* Total number of encoded bytes */
    speck_bytes = speck_bytes_Y + speck_bytes_Cb + speck_bytes_Cr;

    /* Allocate memory for mixed Cb + Cr channel */
    buf_Cb_Cr = (unsigned char *) xmalloc((speck_bytes_Cb + speck_bytes_Cr) *
        sizeof(unsigned char));

    /* Merge Cb and Cr channels */
    merge_channels(buf_Cb, buf_Cr, buf_Cb_Cr, speck_bytes_Cb, speck_bytes_Cr);

    /* No longer needed */
    free(buf_Cb);
    free(buf_Cr);

    /* Allocate memory for mixed Y + (Cb + Cr) channel */
    buf_Y_Cb_Cr = (unsigned char *) xmalloc(speck_bytes *
        sizeof(unsigned char));

    /* Merge Y and (Cb + Cr) channels */
    merge_channels(buf_Y, buf_Cb_Cr, buf_Y_Cb_Cr, speck_bytes_Y,
        speck_bytes_Cb + speck_bytes_Cr);

    /* No longer needed */
    free(buf_Y);
    free(buf_Cb_Cr);

    /* Byte stuffing */
    stuff_max = speck_bytes + speck_bytes / 254 + 1;
    stuff_buf = (unsigned char *) xmalloc(stuff_max * sizeof(unsigned char));
    stuff_bytes = stuff_data(buf_Y_Cb_Cr, stuff_buf, speck_bytes, stuff_max);

    assert(stuff_bytes >= speck_bytes);

    free(buf_Y_Cb_Cr);

    /* Write block header */
    str_len = snprintf((char *) buf_next, bytes_left,
        "type=tc;W=%d;H=%d;w=%d;h=%d;x=%d;y=%d;m=%d;r=%d;"
        "dc=%d:%d:%d;rt=%d:%d:%d;fb=%s;",
        W, H, w, h, x, y, mode, resample,
        dc_Y_int, dc_Cb_int, dc_Cr_int,
        speck_bytes_Y, speck_bytes_Cb,
        speck_bytes_Cr, fb_id);

    assert(str_len < bytes_left);

    buf_next += str_len;
    bytes_left -= str_len;

    /* Compute and save header CRC */
    hdr_crc = epsilon_crc32(buf, str_len);
    hdr_crc = (hdr_crc ^ (hdr_crc >> 16)) & 0xffff;

    str_len = snprintf((char *) buf_next, bytes_left,
                       "chk=%04x;crc=????????;", hdr_crc);

    assert(str_len < bytes_left);

    buf_next += str_len;
    bytes_left -= str_len;

    crc_pos = buf_next - 9;

    /* Cut encoded stream to fit it within available space */
    stuff_cut = MIN(bytes_left, stuff_bytes);
    memcpy(buf_next, stuff_buf, stuff_cut);

    free(stuff_buf);

    /* Compute and save data CRC */
    data_crc = epsilon_crc32(buf_next, stuff_cut);
    snprintf((char *) crc_pos, 9, "%08x", data_crc);
    crc_pos[8] = ';';

    buf_next += stuff_cut;
    bytes_left -= stuff_cut;

    /* Compute real amount of used bytes */
    *buf_size = buf_next - buf;

    return EPS_OK;
}

int eps_decode_truecolor_block(unsigned char **block_R,
                               unsigned char **block_G,
                               unsigned char **block_B,
                               unsigned char *buf,
                               eps_block_header *hdr)
{
    filterbank_t *fb;

    unsigned char *unstuff_buf;
    int unstuff_bytes;

    unsigned char *buf_Y;
    unsigned char *buf_Cb;
    unsigned char *buf_Cr;
    unsigned char *buf_Cb_Cr;

    int speck_bytes_Y;
    int speck_bytes_Cb;
    int speck_bytes_Cr;
    int speck_bytes_Cb_Cr;

    int full_size;
    int half_size;

    int **int_block_Y;
    int **int_block_Cb;
    int **int_block_Cr;

    coeff_t **dwt_block_Y;
    coeff_t **dwt_block_Cb;
    coeff_t **dwt_block_Cr;

    coeff_t **pad_block_Y;
    coeff_t **pad_block_Cb;
    coeff_t **pad_block_Cr;

    coeff_t **pad_block_R;
    coeff_t **pad_block_G;
    coeff_t **pad_block_B;

    coeff_t **block_Y;
    coeff_t **block_Cb;
    coeff_t **block_Cr;

    int block_Y_size;
    int block_Cb_size;
    int block_Cr_size;

    unsigned char dc_Y_int;
    unsigned char dc_Cb_int;
    unsigned char dc_Cr_int;

    /* Sanity checks */
    if (!block_R || !block_G || !block_B) {
        return EPS_PARAM_ERROR;
    }

    if (!buf || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (hdr->data_size < 1) {
        return EPS_PARAM_ERROR;
    }

    /* Reset RGB channels */
    reset_RGB(block_R, block_G, block_B, hdr->hdr_data.tc.w, hdr->hdr_data.tc.h);

    /* Find filterbank by id */
    if (!hdr->hdr_data.tc.fb_id) {
        return EPS_UNSUPPORTED_FB;
    }

    fb = get_fb(hdr->hdr_data.tc.fb_id);
    assert(fb);

    /* Unstaff data */
    unstuff_buf = (unsigned char *) xmalloc(hdr->data_size *
        sizeof(unsigned char));

    unstuff_bytes = unstuff_data(buf + hdr->hdr_size, unstuff_buf,
        hdr->data_size, hdr->data_size);

    /* Consistency check */
    if (unstuff_bytes > hdr->hdr_data.tc.Y_rt + hdr->hdr_data.tc.Cb_rt + hdr->hdr_data.tc.Cr_rt) {
        free(unstuff_buf);
        return EPS_FORMAT_ERROR;
    }

    /* Dummy data */
    if (unstuff_bytes == 0) {
        unstuff_buf[0] = 0;
        unstuff_bytes = 1;
    }

    /* Allocate memory for Y and (Cb + Cr) channels */
    buf_Y = (unsigned char *) xmalloc(unstuff_bytes *
        sizeof(unsigned char));

    buf_Cb_Cr = (unsigned char *) xmalloc(unstuff_bytes *
        sizeof(unsigned char));

    /* Split stream into Y and (Cb + Cr) channels */
    split_channels(unstuff_buf, buf_Y, buf_Cb_Cr,
                   unstuff_bytes, hdr->hdr_data.tc.Y_rt,
                   hdr->hdr_data.tc.Cb_rt + hdr->hdr_data.tc.Cr_rt,
                   &speck_bytes_Y, &speck_bytes_Cb_Cr);

    /* No longer needed */
    free(unstuff_buf);

    /* Consistency check */
    if (speck_bytes_Cb_Cr > hdr->hdr_data.tc.Cb_rt + hdr->hdr_data.tc.Cr_rt) {
        free(buf_Y);
        free(buf_Cb_Cr);
        return EPS_FORMAT_ERROR;
    }

    /* Dummy data */
    if (speck_bytes_Y == 0) {
        buf_Y[0] = 0;
        speck_bytes_Y = 1;
    }

    /* Dummy data */
    if (speck_bytes_Cb_Cr == 0) {
        buf_Cb_Cr[0] = 0;
        speck_bytes_Cb_Cr = 1;
    }

    /* Allocate memory for Cb and Cr channels */
    buf_Cb = (unsigned char *) xmalloc(speck_bytes_Cb_Cr *
        sizeof(unsigned char));

    buf_Cr = (unsigned char *) xmalloc(speck_bytes_Cb_Cr *
        sizeof(unsigned char));

    /* Split merged (Cb + Cr) channel into Cb and Cr channels */
    split_channels(buf_Cb_Cr, buf_Cb, buf_Cr, speck_bytes_Cb_Cr,
                   hdr->hdr_data.tc.Cb_rt, hdr->hdr_data.tc.Cr_rt,
                   &speck_bytes_Cb, &speck_bytes_Cr);

    /* No longer needed */
    free(buf_Cb_Cr);

    /* Dummy data */
    if (speck_bytes_Cb == 0) {
        buf_Cb[0] = 0;
        speck_bytes_Cb = 1;
    }

    /* Dummy data */
    if (speck_bytes_Cr == 0) {
        buf_Cr[0] = 0;
        speck_bytes_Cr = 1;
    }

    /* Compute block sizes for full and resampled channels */
    full_size = get_block_size(hdr->hdr_data.tc.w, hdr->hdr_data.tc.h, hdr->hdr_data.tc.mode, 4);
    half_size = full_size / 2 + (hdr->hdr_data.tc.mode == EPS_MODE_OTLPF);

    if (hdr->hdr_data.tc.resample == EPS_RESAMPLE_444) {
        block_Y_size = full_size;
        block_Cb_size = full_size;
        block_Cr_size = full_size;
    } else {
        block_Y_size = full_size;
        block_Cb_size = half_size;
        block_Cr_size = half_size;
    }

    /* Allocate memory for Y,Cb,Cr channels */
    int_block_Y = (int **) malloc_2D(block_Y_size, block_Y_size,
        sizeof(int));
    int_block_Cb = (int **) malloc_2D(block_Cb_size, block_Cb_size,
        sizeof(int));
    int_block_Cr = (int **) malloc_2D(block_Cr_size, block_Cr_size,
        sizeof(int));

    /* Decode data */
    speck_decode(buf_Y, speck_bytes_Y, int_block_Y, block_Y_size);
    speck_decode(buf_Cb, speck_bytes_Cb, int_block_Cb, block_Cb_size);
    speck_decode(buf_Cr, speck_bytes_Cr, int_block_Cr, block_Cr_size);

    /* No longer needed */
    free(buf_Y);
    free(buf_Cb);
    free(buf_Cr);

    /* Allocate memory for real-valued wavelet coefficients */
    dwt_block_Y = (coeff_t **) malloc_2D(block_Y_size, block_Y_size,
        sizeof(coeff_t));
    dwt_block_Cb = (coeff_t **) malloc_2D(block_Cb_size, block_Cb_size,
        sizeof(coeff_t));
    dwt_block_Cr = (coeff_t **) malloc_2D(block_Cr_size, block_Cr_size,
        sizeof(coeff_t));

    /* Copy data with type extension */
    copy_channel(int_block_Y, dwt_block_Y, block_Y_size);
    copy_channel(int_block_Cb, dwt_block_Cb, block_Cb_size);
    copy_channel(int_block_Cr, dwt_block_Cr, block_Cr_size);

    /* No longer needed */
    free_2D((void *) int_block_Y, block_Y_size, block_Y_size);
    free_2D((void *) int_block_Cb, block_Cb_size, block_Cb_size);
    free_2D((void *) int_block_Cr, block_Cr_size, block_Cr_size);

    /* Allocate memory for restored Y,Cb,Cr channels */
    block_Y = (coeff_t **) malloc_2D(block_Y_size, block_Y_size,
        sizeof(coeff_t));
    block_Cb = (coeff_t **) malloc_2D(block_Cb_size, block_Cb_size,
        sizeof(coeff_t));
    block_Cr = (coeff_t **) malloc_2D(block_Cr_size, block_Cr_size,
        sizeof(coeff_t));

    /* Inverse wavelet transform */
    synthesis_2D(dwt_block_Y, block_Y, block_Y_size, hdr->hdr_data.tc.mode, fb);
    synthesis_2D(dwt_block_Cb, block_Cb, block_Cb_size, hdr->hdr_data.tc.mode, fb);
    synthesis_2D(dwt_block_Cr, block_Cr, block_Cr_size, hdr->hdr_data.tc.mode, fb);

    /* No longer needed */
    free_2D((void *) dwt_block_Y, block_Y_size, block_Y_size);
    free_2D((void *) dwt_block_Cb, block_Cb_size, block_Cb_size);
    free_2D((void *) dwt_block_Cr, block_Cr_size, block_Cr_size);

    /* Get DC values */
    dc_Y_int  = (unsigned char) hdr->hdr_data.tc.dc_Y;
    dc_Cb_int = (unsigned char) hdr->hdr_data.tc.dc_Cb;
    dc_Cr_int = (unsigned char) hdr->hdr_data.tc.dc_Cr;

    /* DC level unshift */
    dc_level_unshift(block_Y, (coeff_t) dc_Y_int,
        block_Y_size, block_Y_size);
    dc_level_unshift(block_Cb, (coeff_t) dc_Cb_int,
        block_Cb_size, block_Cb_size);
    dc_level_unshift(block_Cr, (coeff_t) dc_Cr_int,
        block_Cr_size, block_Cr_size);

    if (hdr->hdr_data.tc.resample == EPS_RESAMPLE_444) {
        /* No upsampling */
        pad_block_Y = block_Y;
        pad_block_Cb = block_Cb;
        pad_block_Cr = block_Cr;
    } else {
        pad_block_Y = block_Y;

        /* Allocate memory for full-sized Cb and Cr channels */
        pad_block_Cb = (coeff_t **) malloc_2D(full_size, full_size,
            sizeof(coeff_t));

        pad_block_Cr = (coeff_t **) malloc_2D(full_size, full_size,
            sizeof(coeff_t));

        /* Upsample Cb and Cr channels according to 4:2:0 scheme */
        bilinear_resample_channel(block_Cb, pad_block_Cb,
                                  block_Cb_size, block_Cb_size,
                                  full_size, full_size);

        bilinear_resample_channel(block_Cr, pad_block_Cr,
                                  block_Cr_size, block_Cr_size,
                                  full_size, full_size);

        /* No longer needed */
        free_2D((void *) block_Cb, block_Cb_size, block_Cb_size);
        free_2D((void *) block_Cr, block_Cr_size, block_Cr_size);
    }

    /* Allocate memory for extended R,G,B channels */
    pad_block_R = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_G = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));
    pad_block_B = (coeff_t **) malloc_2D(full_size, full_size,
        sizeof(coeff_t));

    /* Convert from Y,Cb,Cr to R,G,B color space */
    convert_YCbCr_to_RGB(pad_block_Y, pad_block_Cb, pad_block_Cr,
                         pad_block_R, pad_block_G, pad_block_B,
                         full_size, full_size);

    /* No longer needed */
    free_2D((void *) pad_block_Y, full_size, full_size);
    free_2D((void *) pad_block_Cb, full_size, full_size);
    free_2D((void *) pad_block_Cr, full_size, full_size);

    /* Clip R,G,B channels */
    clip_channel(pad_block_R, full_size, full_size);
    clip_channel(pad_block_G, full_size, full_size);
    clip_channel(pad_block_B, full_size, full_size);

    /* Extract original data from R,G,B channels */
    extract_channel(pad_block_R, block_R,
                    full_size, full_size,
                    hdr->hdr_data.tc.w,
                    hdr->hdr_data.tc.h);

    extract_channel(pad_block_G, block_G,
                    full_size, full_size,
                    hdr->hdr_data.tc.w,
                    hdr->hdr_data.tc.h);

    extract_channel(pad_block_B, block_B,
                    full_size, full_size,
                    hdr->hdr_data.tc.w,
                    hdr->hdr_data.tc.h);

    /* No longer needed */
    free_2D((void *) pad_block_R, full_size, full_size);
    free_2D((void *) pad_block_G, full_size, full_size);
    free_2D((void *) pad_block_B, full_size, full_size);

    return EPS_OK;
}

int eps_truncate_block(unsigned char *buf_in, unsigned char *buf_out,
                       eps_block_header *hdr, int *truncate_size)
{
    crc32_t data_crc;

    /* Sanity checks */
    if (!buf_in || !buf_out || !hdr) {
        return EPS_PARAM_ERROR;
    }

    if (*truncate_size < MAX(EPS_MIN_GRAYSCALE_BUF, EPS_MIN_TRUECOLOR_BUF)) {
        return EPS_PARAM_ERROR;
    }

    /* Copy data */
    *truncate_size = MIN(*truncate_size, hdr->hdr_size + hdr->data_size);
    memcpy(buf_out, buf_in, *truncate_size);

    /* Recompute data CRC */
    data_crc = epsilon_crc32(buf_out + hdr->hdr_size, *truncate_size - hdr->hdr_size);
    snprintf((char *) (buf_out + hdr->hdr_size - 9), 9, "%08x", data_crc);
    *(buf_out + hdr->hdr_size - 1) = ';';

    return EPS_OK;
}
