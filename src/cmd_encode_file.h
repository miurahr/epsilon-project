/*
 * $Id: cmd_encode_file.h,v 1.33 2011/04/28 08:57:32 simakov Exp $
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

#ifndef __CMD_ENCODE_FILE_H__
#define __CMD_ENCODE_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_PTHREADS
# include <pthread.h>
#endif

#ifdef ENABLE_CLUSTER
# include <sys/socket.h>
# include <sys/types.h>
#endif

#include <pbm.h>
#include <psi.h>
#include <time.h>

#define ORTHOGONAL              0
#define BIORTHOGONAL            1

/* Encoding context for multi-theaded environment.
 * This code is designed to be compatible with
 * thread-unaware program version. */
typedef struct encode_ctx_tag {
    char *filter_id;
    int block_size;
    int mode;
    int bytes_per_block;
    double Y_ratio;
    double Cb_ratio;
    double Cr_ratio;
    int resample;
    int W;
    int H;
    int n_blocks;
    int *done_blocks;
    int n_threads;
#ifdef ENABLE_CLUSTER
    struct sockaddr_in *node;
#endif
    int thread_idx;
    time_t start_time;
    char *pbm_file;
    char *psi_file;
    pbm_image *pbm;
    psi_image *psi;
    int current;
    int total;
    int quiet;
    int *clear_len;
    int *stop_flag;
} encode_ctx;

static int check_pbm_ext(char *file);
static void replace_pbm_to_psi(char *file);
static void *encode_blocks(void *arg);
static void encode_file(char *filter_id, int block_size, int mode,
                        double ratio, int two_pass, int n_threads,
                        void *cluster, int Y_ratio, int Cb_ratio,
                        int Cr_ratio, int resample, int halt_on_errors,
                        int quiet, char *output_dir, char *file,
                        int current, int total);

#ifdef ENABLE_MPI
static void encode_file_mpi(char *filter_id, int block_size, int mode,
                            double ratio, int two_pass, int n_threads,
                            int Y_ratio, int Cb_ratio, int Cr_ratio,
                            int resample, int halt_on_errors,
                            int quiet, char *output_dir, char *file,
                            int current, int total);
#endif

void cmd_encode_file(char *filter_id, int block_size, int mode,
                     double ratio, int two_pass, int n_threads,
                     char *node_list, int Y_ratio, int Cb_ratio,
                     int Cr_ratio, int resample, int halt_on_errors,
                     int quiet, char *output_dir, char **files);

#ifdef __cplusplus
}
#endif

#endif /* __CMD_ENCODE_FILE_H__ */
