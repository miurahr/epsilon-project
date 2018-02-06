/*
 * $Id: pbm.c,v 1.19 2010/03/19 22:57:29 simakov Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pbm.h>
#include <misc.h>

/* Get file size */
int get_file_size(char *pathname, off_t *file_size)
{
    struct stat st;

    if (stat(pathname, &st) == -1) {
        return PBM_SYSTEM_ERROR;
    }

    *file_size = st.st_size;

    return PBM_OK;
}

/* fseeko wrapper: either SEEK_SET or SEEK_CUR mode */
static int fseeko_jumbo(FILE *f, off_t offset, int whence) {
    /* Set pointer at start position in SEEK_SET mode */
    if (whence == SEEK_SET) {
        if (fseeko(f, 0, SEEK_SET) == -1) {
            return PBM_SYSTEM_ERROR;
        }
    }

    /* This is fseeko workaround for 32-bit machines.
     * It seems that second argument of fseeko (despite
     * of off_t type) cannot exceed the value of 2^31 - 1.
     * That`s why we need a loop here. */
    while (offset > 0) {
        int jumbo = MIN(JUMBO_JUMP, offset);

        if (fseeko(f, jumbo, SEEK_CUR) == -1) {
            return PBM_SYSTEM_ERROR;
        }

        offset -= jumbo;
    }

    return PBM_OK;
}

/* Read next char */
static int get_char(FILE *f)
{
    int ch;

    ch = getc(f);

    /* Skip over comment */
    if (ch == '#') {
        do {
            ch = getc(f);
        } while ((ch != '\n') && (ch != EOF));
    }

    return ch;
}

/* Read next integer */
static int get_integer(FILE *f, int *val)
{
    int ch;

    /* Skip over leading space symbols */
    do {
        if ((ch = get_char(f)) == EOF) {
            return PBM_FORMAT_ERROR;
        }
    } while ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'));

    /* Read first digit */
    if ((ch < '0') || (ch > '9')) {
        return PBM_FORMAT_ERROR;
    }

    *val = ch - '0';

    /* Read digits */
    for (;;) {
        if ((ch = get_char(f)) == EOF) {
            return PBM_FORMAT_ERROR;
        }

        if ((ch < '0') || (ch > '9')) {
            break;
        }

        *val *= 10;
        *val += ch - '0';
    }

    return PBM_OK;
}

/* Open PBM file */
int pbm_open(char *pathname, pbm_image *pbm)
{
    off_t file_size;
    off_t hdr_size;
    int rc;

    pbm->f = NULL;
    pbm->f = fopen(pathname, "rb");

    if (pbm->f == NULL) {
        return PBM_SYSTEM_ERROR;
    }

    rc = get_file_size(pathname, &file_size);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Read and check file header */
    if (getc(pbm->f) != 'P') {
        return PBM_FORMAT_ERROR;
    }

    switch (getc(pbm->f)) {
        case '5':
            pbm->type = PBM_TYPE_PGM;
            break;
        case '6':
            pbm->type = PBM_TYPE_PPM;
            break;
        default:
            return PBM_FORMAT_ERROR;
    }

    /* Get width */
    rc = get_integer(pbm->f, &pbm->width);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Get height */
    rc = get_integer(pbm->f, &pbm->height);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Get maximal value */
    rc = get_integer(pbm->f, &pbm->max_val);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Check params for consistency */
    if ((pbm->width <= 0) || (pbm->height <= 0)) {
        return PBM_FORMAT_ERROR;
    }

    if ((pbm->max_val < 0) || (pbm->max_val > 255)) {
        return PBM_FORMAT_ERROR;
    }

    /* Compute precise header and data size */
    if ((hdr_size = ftello(pbm->f)) == _OFF(-1)) {
        return PBM_SYSTEM_ERROR;
    }

    pbm->hdr_size = hdr_size;
    pbm->data_size = _OFF(pbm->width) * _OFF(pbm->height) *
        _OFF(pbm->type == PBM_TYPE_PGM ? 1 : 3);

    if (file_size != pbm->hdr_size + pbm->data_size) {
        return PBM_FORMAT_ERROR;
    }

    return PBM_OK;
}

/* Create PBM file */
int pbm_create(char *pathname, pbm_image *pbm)
{
    int rc;

    pbm->f = NULL;

    if ((pbm->type != PBM_TYPE_PGM) && (pbm->type != PBM_TYPE_PPM)) {
        return PBM_FORMAT_ERROR;
    }

    if ((pbm->width <= 0) || (pbm->height <= 0)) {
        return PBM_FORMAT_ERROR;
    }

    /* Create file */
    pbm->f = fopen(pathname, "wb+");

    if (pbm->f == NULL) {
        return PBM_SYSTEM_ERROR;
    }

    /* Write file header */
    pbm->hdr_size = fprintf(pbm->f, "P%c\n%d %d\n255\n",
        pbm->type == PBM_TYPE_PGM ? '5' : '6',
        pbm->width, pbm->height);

    pbm->data_size = _OFF(pbm->width) * _OFF(pbm->height) *
        _OFF((pbm->type == PBM_TYPE_PGM ? 1 : 3));

    pbm->max_val = 255;

    /* Prepare blank image */
    rc = fseeko_jumbo(pbm->f, _OFF(pbm->data_size) - _OFF(1), SEEK_CUR);

    if (rc != PBM_OK) {
        return rc;
    }

    if (fputc(0, pbm->f) == EOF) {
        return PBM_SYSTEM_ERROR;
    }

    return PBM_OK;
}

/* Close PBM file */
void pbm_close(pbm_image *pbm)
{
    if (pbm->f) {
        fclose(pbm->f);
    }
}

/* Read block from PGM file */
int pbm_read_pgm(pbm_image *pbm, unsigned char **block,
                 int x, int y, int width, int height)
{
    off_t offset;
    int i, j;
    int rc;

    /* Check params for consistency */
    if ((x < 0) || (y < 0)) {
        return PBM_PARAM_ERROR;
    }

    if ((x >= pbm->width) || (y >= pbm->height)) {
        return PBM_PARAM_ERROR;
    }

    if ((width <= 0) || (height <= 0)) {
        return PBM_PARAM_ERROR;
    }

    if (x + width > pbm->width) {
        return PBM_PARAM_ERROR;
    }

    if (y + height > pbm->height) {
        return PBM_PARAM_ERROR;
    }

    /* Set file pointer */
    offset = _OFF(pbm->hdr_size) + _OFF(y) * _OFF(pbm->width) + _OFF(x);
    rc = fseeko_jumbo(pbm->f, offset, SEEK_SET);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Fetch desired block from the file */
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            int ch = fgetc(pbm->f);

            if (ch != EOF) {
                block[j][i] = (unsigned char) ch;
            } else {
                return PBM_SYSTEM_ERROR;
            }
        }

        /* Next scanline */
        offset = _OFF(_OFF(pbm->width) - _OFF(width));
        rc = fseeko_jumbo(pbm->f, offset, SEEK_CUR);

        if (rc != PBM_OK) {
            return rc;
        }
    }

    return PBM_OK;
}

/* Write block into the PGM file */
int pbm_write_pgm(pbm_image *pbm, unsigned char **block,
                  int x, int y, int width, int height)
{
    off_t offset;
    int i, j;
    int rc;

    /* Check params for consistency */
    if ((x < 0) || (y < 0)) {
        return PBM_PARAM_ERROR;
    }

    if ((x >= pbm->width) || (y >= pbm->height)) {
        return PBM_PARAM_ERROR;
    }

    if ((width <= 0) || (height <= 0)) {
        return PBM_PARAM_ERROR;
    }

    if (x + width > pbm->width) {
        return PBM_PARAM_ERROR;
    }

    if (y + height > pbm->height) {
        return PBM_PARAM_ERROR;
    }

    offset = _OFF(pbm->hdr_size) + _OFF(y) * _OFF(pbm->width) + _OFF(x);
    rc = fseeko_jumbo(pbm->f, offset, SEEK_SET);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Store desired block */
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            if (fputc(block[j][i], pbm->f) == EOF) {
                return PBM_SYSTEM_ERROR;
            }
        }

        /* Next scanline */
        offset = _OFF(pbm->width) - _OFF(width);
        rc = fseeko_jumbo(pbm->f, offset, SEEK_CUR);

        if (rc != PBM_OK) {
            return rc;
        }
    }

    return PBM_OK;
}

/* Read block from PPM file */
int pbm_read_ppm(pbm_image *pbm, unsigned char **block_R,
                 unsigned char **block_G, unsigned char **block_B,
                 int x, int y, int width, int height)
{
    off_t offset;
    int i, j;
    int rc;

    /* Check params for consistency */
    if ((x < 0) || (y < 0)) {
        return PBM_PARAM_ERROR;
    }

    if ((x >= pbm->width) || (y >= pbm->height)) {
        return PBM_PARAM_ERROR;
    }

    if ((width <= 0) || (height <= 0)) {
        return PBM_PARAM_ERROR;
    }

    if (x + width > pbm->width) {
        return PBM_PARAM_ERROR;
    }

    if (y + height > pbm->height) {
        return PBM_PARAM_ERROR;
    }

    offset = _OFF(pbm->hdr_size) + (_OFF(y) * _OFF(pbm->width) + _OFF(x)) * _OFF(3);
    rc = fseeko_jumbo(pbm->f, offset, SEEK_SET);

    if (rc != PBM_OK) {
        return PBM_SYSTEM_ERROR;
    }

    /* Fetch desired block */
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            int ch;

            if ((ch = fgetc(pbm->f)) != EOF) {
                block_R[j][i] = (unsigned char) ch;
            } else {
                return PBM_SYSTEM_ERROR;
            }

            if ((ch = fgetc(pbm->f)) != EOF) {
                block_G[j][i] = (unsigned char) ch;
            } else {
                return PBM_SYSTEM_ERROR;
            }

            if ((ch = fgetc(pbm->f)) != EOF) {
                block_B[j][i] = (unsigned char) ch;
            } else {
                return PBM_SYSTEM_ERROR;
            }
        }

        /* Next scanline */
        offset = (_OFF(pbm->width) - _OFF(width)) * _OFF(3);
        rc = fseeko_jumbo(pbm->f, offset, SEEK_CUR);

        if (rc != PBM_OK) {
            return rc;
        }
    }

    return PBM_OK;
}

/* Write block into the PPM file */
int pbm_write_ppm(pbm_image *pbm, unsigned char **block_R,
                  unsigned char **block_G, unsigned char **block_B,
                  int x, int y, int width, int height)
{
    off_t offset;
    int i, j;
    int rc;

    /* Check params for consistency */
    if ((x < 0) || (y < 0)) {
        return PBM_PARAM_ERROR;
    }

    if ((x >= pbm->width) || (y >= pbm->height)) {
        return PBM_PARAM_ERROR;
    }

    if ((width <= 0) || (height <= 0)) {
        return PBM_PARAM_ERROR;
    }

    if (x + width > pbm->width) {
        return PBM_PARAM_ERROR;
    }

    if (y + height > pbm->height) {
        return PBM_PARAM_ERROR;
    }

    offset = _OFF(pbm->hdr_size) + (_OFF(y) * _OFF(pbm->width) + _OFF(x)) * _OFF(3);
    rc = fseeko_jumbo(pbm->f, offset, SEEK_SET);

    if (rc != PBM_OK) {
        return rc;
    }

    /* Store desired block */
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            if (fputc(block_R[j][i], pbm->f) == EOF) {
                return PBM_SYSTEM_ERROR;
            }

            if (fputc(block_G[j][i], pbm->f) == EOF) {
                return PBM_SYSTEM_ERROR;
            }

            if (fputc(block_B[j][i], pbm->f) == EOF) {
                return PBM_SYSTEM_ERROR;
            }
        }

        offset = (_OFF(pbm->width) - _OFF(width)) * _OFF(3);
        rc = fseeko_jumbo(pbm->f, offset, SEEK_CUR);

        if (rc != PBM_OK) {
            return rc;
        }
    }

    return PBM_OK;
}
