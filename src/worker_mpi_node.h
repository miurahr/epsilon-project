/*
 * $Id: worker_mpi_node.h,v 1.15 2010/02/05 23:50:22 simakov Exp $
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

#ifndef __WORKER_MPI_NODE_H__
#define __WORKER_MPI_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_MPI

#include <mpi.h>

/* Command list */
#define CMD_MPI_SHUTDOWN        0
#define CMD_MPI_ENCODE_GS       1
#define CMD_MPI_DECODE_GS       2
#define CMD_MPI_ENCODE_TC       3
#define CMD_MPI_DECODE_TC       4

/* Some handy shortcuts */
#define FREE_DUMMY_HANDLE()     assert(MPI_Request_free((MPI_Request *) &dummy) == MPI_SUCCESS);

/* MPI context structure */
typedef struct MPI_ctx_tag {
    int x;
    int y;
    int w;
    int h;
    unsigned char *Y0;
    unsigned char *R0;
    unsigned char *G0;
    unsigned char *B0;
    unsigned char *data;
    int data_size;
    int is_free;
} MPI_ctx;

MPI_ctx *alloc_MPI_ctx(int width, int height, int bytes_per_block, int size, int type);
void free_MPI_ctx(MPI_ctx *ctx, int size, int type);
MPI_Request *alloc_MPI_req(int size);
void free_MPI_req(MPI_Request *req);
void shutdown_nodes();
static void cmd_mpi_encode_gs();
static void cmd_mpi_encode_tc();
static void cmd_mpi_decode_gs();
static void cmd_mpi_decode_tc();
void worker_mpi_node(int rank);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __WORKER_MPI_NODE_H__ */
