/*
 * $Id: psi.h,v 1.9 2010/02/05 23:50:22 simakov Exp $
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

#ifndef __PSI_H__
#define __PSI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <pbm.h>

/* Return codes */
#define PSI_OK                  0
#define PSI_EOF                 1
#define PSI_SYSTEM_ERROR        2
#define PSI_GUESS_ERROR         3

/* PSI image context */
typedef struct psi_image_tag {
    FILE *f;
    int max_block_w;
    int max_block_h;
} psi_image;

int psi_open(char *pathname, psi_image *psi);
int psi_create(char *pathname, psi_image *psi);
void psi_close(psi_image *psi);
int psi_read_next_block(psi_image *psi, unsigned char *buf, int *buf_size);
int psi_write_next_block(psi_image *psi, unsigned char *buf, int buf_size);
int psi_guess_pbm_type(psi_image *psi, pbm_image *pbm);

#ifdef __cplusplus
}
#endif

#endif /* __PSI_H__ */
