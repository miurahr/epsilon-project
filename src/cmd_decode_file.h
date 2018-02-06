/*
 * $Id: cmd_decode_file.h,v 1.18 2011/04/28 08:57:31 simakov Exp $
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

#ifndef __CMD_DECODE_FILE_H__
#define __CMD_DECODE_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

/* Decoding context for multi-theaded environment.
 * This code is designed to be compatible with
 * thread-unaware program version. */
typedef struct decode_ctx_tag {
    int buf_size;
    int n_blocks;
    int *done_blocks;
    int W;
    int H;
    int *clear_len;
    time_t start_time;
    char *psi_file;
    char *pbm_file;
    psi_image *psi;
    pbm_image *pbm;
#ifdef ENABLE_CLUSTER
    struct sockaddr_in *node;
#endif
    int current;
    int total;
    int ignore_hdr_crc;
    int ignore_data_crc;
    int ignore_format_err;
    int quiet;
    int *stop_flag;
} decode_ctx;

int check_psi_ext(char *file);
static void replace_psi_to_pbm(char *file, int pbm_type);
static void *decode_blocks(void *arg);
static void decode_file(int n_threads, void *cluster, int halt_on_errors,
                        int quiet, int ignore_hdr_crc, int ignore_data_crc,
                        int ignore_format_err, char *file,
                        char *output_dir, int current, int total);

#ifdef ENABLE_MPI
void cmd_decode_file_mpi(int n_threads, char *node_list, int halt_on_errors,
                         int quiet, int ignore_hdr_crc, int ignore_data_crc,
                         int ignore_format_err, char *output_dir,
                         char **files);
#endif

void cmd_decode_file(int n_threads, char *node_list, int halt_on_errors,
                     int quiet, int ignore_hdr_crc, int ignore_data_crc,
                     int ignore_format_err, char *output_dir,
                     char **files);

#ifdef __cplusplus
}
#endif

#endif /* __CMD_DECODE_FILE_H__ */
