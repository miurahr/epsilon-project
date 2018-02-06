/*
 * $Id: psi.c,v 1.13 2010/02/05 23:50:22 simakov Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <epsilon.h>
#include <psi.h>
#include <pbm.h>
#include <misc.h>

/* Open PSI file */
int psi_open(char *pathname, psi_image *psi)
{
    psi->f = NULL;
    psi->f = fopen(pathname, "rb");

    if (psi->f == NULL) {
        return PSI_SYSTEM_ERROR;
    }

    return PSI_OK;
}

/* Create PSI file */
int psi_create(char *pathname, psi_image *psi)
{
    psi->f = NULL;
    psi->f = fopen(pathname, "wb");

    if (psi->f == NULL) {
        return PSI_SYSTEM_ERROR;
    }

    return PSI_OK;
}

/* Close PSI file */
void psi_close(psi_image *psi)
{
    if (psi->f) {
        fclose(psi->f);
    }
}

/* Read next encoded block */
int psi_read_next_block(psi_image *psi, unsigned char *buf, int *buf_size)
{
    unsigned char *next_byte;
    int bytes_left;
    int ch;

    /* Copy no more than *buf_size bytes */
    bytes_left = *buf_size;
    next_byte = buf;

    /* Find first non-marker byte */
    for (;;) {
        ch = fgetc(psi->f);

        if (ch == EOF) {
            return PSI_EOF;
        }

        if (ch != EPS_MARKER) {
            *next_byte++ = (unsigned char) ch;
            bytes_left--;
            break;
        }
    }

    /* Copy data until next marker, EOF or buffer end */
    for (;;) {
        ch = fgetc(psi->f);

        if ((ch == EOF) || (ch == EPS_MARKER)) {
            break;
        }

        /* No more space in the buffer or currupted data.
         * Continue until EOF or syncronization marker */
        if (!bytes_left) {
            continue;
        }

        *next_byte++ = (unsigned char) ch;
        bytes_left--;
    }

    /* Actual number of read bytes */
    *buf_size = next_byte - buf;

    return PSI_OK;
}

/* Write next encoded block */
int psi_write_next_block(psi_image *psi, unsigned char *buf, int buf_size)
{
    /* Write data */
    if (fwrite(buf, 1, buf_size, psi->f) != buf_size) {
        return PSI_SYSTEM_ERROR;
    }

    /* Write syncronization marker */
    if (fputc(EPS_MARKER, psi->f) == EOF) {
        return PSI_SYSTEM_ERROR;
    }

    return PSI_OK;
}

/* Try to guess target file type (PGM or PPM) and
 * main characteristics. NB: you should call this
 * function prior to actual decoding. */
int psi_guess_pbm_type(psi_image *psi, pbm_image *pbm)
{
    /* Block buffer */
    unsigned char *buf;
    int buf_size;

    /* Image characteristics */
    int W, H, w, h;
    int type;

    /* Allocate block buffer */
    buf_size = MAX(EPS_MAX_GRAYSCALE_BUF, EPS_MAX_TRUECOLOR_BUF);
    buf = (unsigned char *) eps_xmalloc(buf_size * sizeof(unsigned char));

    W = H = w = h = type = -1;

    /* For all blocks */
    while (1) {
        eps_block_header hdr;

        /* Read next block */
        if (psi_read_next_block(psi, buf, &buf_size) != PSI_OK) {
            break;
        }

        /* Parse block header */
        if (eps_read_block_header(buf, buf_size, &hdr) != EPS_OK) {
            continue;
        }

        /* Check block header CRC */
        if (hdr.chk_flag == EPS_BAD_CRC) {
            continue;
        }

        /* Image type */
        if (type == -1) {
            type = hdr.block_type;
        } else {
            if (type != hdr.block_type) {
                rewind(psi->f);
                free(buf);
                return PSI_GUESS_ERROR;
            }
        }

        /* Image width */
        if (W == -1) {
            W = type == EPS_GRAYSCALE_BLOCK ? hdr.hdr_data.gs.W : hdr.hdr_data.tc.W;
        } else {
            if (type == EPS_GRAYSCALE_BLOCK ? W != hdr.hdr_data.gs.W : W != hdr.hdr_data.tc.W) {
                rewind(psi->f);
                free(buf);
                return PSI_GUESS_ERROR;
            }
        }

        /* Image height */
        if (H == -1) {
            H = type == EPS_GRAYSCALE_BLOCK ? hdr.hdr_data.gs.H : hdr.hdr_data.tc.H;
        } else {
            if (type == EPS_GRAYSCALE_BLOCK ? H != hdr.hdr_data.gs.H : H != hdr.hdr_data.tc.H) {
                rewind(psi->f);
                free(buf);
                return PSI_GUESS_ERROR;
            }
        }

        /* Maximal block width and height */
        if (type == EPS_GRAYSCALE_BLOCK) {
            if (hdr.hdr_data.gs.w > w) w = hdr.hdr_data.gs.w;
            if (hdr.hdr_data.gs.h > h) h = hdr.hdr_data.gs.h;
        } else {
            if (hdr.hdr_data.tc.w > w) w = hdr.hdr_data.tc.w;
            if (hdr.hdr_data.tc.h > h) h = hdr.hdr_data.tc.h;
        }
    }

    /* Rewind file and free buffer */
    rewind(psi->f);
    free(buf);

    /* Buggy file */
    if (type == -1 || W == -1 || H == -1 || w == -1 || h == -1) {
        return PSI_GUESS_ERROR;
    }

    /* Save results */
    pbm->type = type == EPS_GRAYSCALE_BLOCK ? PBM_TYPE_PGM : PBM_TYPE_PPM;
    pbm->width = W;
    pbm->height = H;
    psi->max_block_w = w;
    psi->max_block_h = h;

    return PSI_OK;
}
