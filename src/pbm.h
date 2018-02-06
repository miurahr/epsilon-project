/*
 * $Id: pbm.h,v 1.16 2010/03/19 22:57:29 simakov Exp $
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

#ifndef __PBM_H__
#define __PBM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifdef _MSC_VER
#define off_t long
#define ftello ftell
#define fseeko fseek
#endif

/* PBM file type */
#define PBM_TYPE_PGM            0
#define PBM_TYPE_PPM            1

/* Return codes */
#define PBM_OK                  0
#define PBM_FORMAT_ERROR        1
#define PBM_SYSTEM_ERROR        2
#define PBM_PARAM_ERROR         3

/* PBM file context */
typedef struct pbm_image_tag {
    int type;
    int width;
    int height;
    int max_val;
    FILE *f;
    off_t hdr_size;
    off_t data_size;
} pbm_image;

int get_file_size(char *pathname, off_t *file_size);
static int fseeko_jumbo(FILE *f, off_t offset, int whence);
static int get_char(FILE *f);
static int get_integer(FILE *f, int *val);
int pbm_open(char *pathname, pbm_image *pbm);
int pbm_create(char *pathname, pbm_image *pbm);
void pbm_close(pbm_image *pbm);
int pbm_read_pgm(pbm_image *pbm, unsigned char **block,
                 int x, int y, int width, int height);
int pbm_write_pgm(pbm_image *pbm, unsigned char **block,
                  int x, int y, int width, int height);
int pbm_read_ppm(pbm_image *pbm, unsigned char **block_R,
                 unsigned char **block_G, unsigned char **block_B,
                 int x, int y, int width, int height);
int pbm_write_ppm(pbm_image *pbm, unsigned char **block_R,
                  unsigned char **block_G, unsigned char **block_B,
                  int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* __PBM_H__ */
