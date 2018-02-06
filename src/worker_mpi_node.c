/*
 * $Id: worker_mpi_node.c,v 1.17 2010/02/05 23:50:22 simakov Exp $
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

#ifdef ENABLE_MPI

#include <mpi.h>
#include <worker_mpi_node.h>
#include <epsilon.h>
#include <misc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pbm.h>

/* Allocate MPI context array */
MPI_ctx *alloc_MPI_ctx(int width, int height, int bytes_per_block, int size, int type)
{
    MPI_ctx *ctx;
    int i;

    ctx = (MPI_ctx *) xmalloc(size * sizeof(MPI_ctx));

    for (i = 0; i < size; i++) {
        if (type == PBM_TYPE_PGM) {
            ctx[i].Y0 = (unsigned char *) xmalloc(width * height);
        } else {
            ctx[i].R0 = (unsigned char *) xmalloc(width * height);
            ctx[i].G0 = (unsigned char *) xmalloc(width * height);
            ctx[i].B0 = (unsigned char *) xmalloc(width * height);
        }

        ctx[i].data = (unsigned char *) xmalloc(bytes_per_block);
        ctx[i].is_free = 1;
    }

    return ctx;
}

/* Free MPI context array */
void free_MPI_ctx(MPI_ctx *ctx, int size, int type)
{
    int i;

    for (i = 0; i < size; i++) {
        if (type == PBM_TYPE_PGM) {
            free(ctx[i].Y0);
        } else {
            free(ctx[i].R0);
            free(ctx[i].G0);
            free(ctx[i].B0);
        }

        free(ctx[i].data);
    }

    free(ctx);
}

/* Allocate and reset MPI request handle array */
MPI_Request *alloc_MPI_req(int size)
{
    MPI_Request *req;
    int i;

    req = (MPI_Request *) xmalloc(size * sizeof(MPI_Request));
    memset(req, 0, size * sizeof(MPI_Request));

    return req;
}

/* Free MPI request handle array */
void free_MPI_req(MPI_Request *req)
{
    free(req);
}

/* Shutdown all SLAVE nodes */
void shutdown_nodes() {
    int rank, size;
    int cmd = CMD_MPI_SHUTDOWN;
    int i;

    /* Get my rank and universe size */
    assert(MPI_Comm_rank(MPI_COMM_WORLD, &rank) == MPI_SUCCESS);
    assert(MPI_Comm_size(MPI_COMM_WORLD, &size) == MPI_SUCCESS);

    /* Shutdown SLAVE nodes */
    if (rank == 0) {
        for (i = 1; i < size; i++) {
            MPI_Send(&cmd, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
}

/* Encode GS block */
static void cmd_mpi_encode_gs() {
    unsigned char *Y0;
    unsigned char **Y;
    unsigned char *data;

    int W, H;
    int w, h;
    int x, y;

    int bytes_per_block;
    char filter_id[64];
    int mode;

    MPI_Status status;

    /* Receive parameters */
    assert(MPI_Recv(&W, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&H, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&w, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&h, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&y, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&bytes_per_block, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(filter_id, sizeof(filter_id), MPI_UNSIGNED_CHAR,
        MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
        (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&mode, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    /* Allocate memory buffers */
    Y0 = (unsigned char *) xmalloc(w * h);
    Y = (unsigned char **) eps_malloc_2D(w, h, sizeof(unsigned char));
    data = (unsigned char *) xmalloc(bytes_per_block);

    /* Receive image data */
    assert(MPI_Recv(Y0, w * h, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    /* Transform and encode data */
    transform_1D_to_2D(Y0, Y, w, h);
    assert(eps_encode_grayscale_block(Y, W, H, w, h, x, y,
        data, &bytes_per_block, filter_id, mode) == EPS_OK);

    /* Send encoded data to the MASTER node */
    assert(MPI_Send(&bytes_per_block, 1, MPI_INT,
        0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Send(data, bytes_per_block, MPI_UNSIGNED_CHAR,
        0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    /* Free memory buffers */
    free(Y0);
    eps_free_2D((void **) Y, w, h);
    free(data);
}

/* Encode TC block */
static void cmd_mpi_encode_tc() {
    unsigned char *Y0;

    unsigned char **R;
    unsigned char **G;
    unsigned char **B;
    unsigned char *data;

    int W, H;
    int w, h;
    int x, y;

    int resample;
    int bytes_per_block;

    int Y_ratio;
    int Cb_ratio;
    int Cr_ratio;

    char filter_id[64];
    int mode;

    MPI_Status status;

    /* Receive parameters */
    assert(MPI_Recv(&W, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&H, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&w, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&h, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&y, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&resample, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&bytes_per_block, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&Y_ratio, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&Cb_ratio, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&Cr_ratio, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(filter_id, sizeof(filter_id), MPI_UNSIGNED_CHAR,
        MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
        (MPI_Status *) &status) == MPI_SUCCESS);
    assert(MPI_Recv(&mode, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
        MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    /* Allocate memory buffers */
    Y0 = (unsigned char *) xmalloc(w * h);

    R = (unsigned char **) eps_malloc_2D(w, h, sizeof(unsigned char));
    G = (unsigned char **) eps_malloc_2D(w, h, sizeof(unsigned char));
    B = (unsigned char **) eps_malloc_2D(w, h, sizeof(unsigned char));

    data = (unsigned char *) xmalloc(bytes_per_block);

    /* Receive and transform R,G and B image channels */
    assert(MPI_Recv(Y0, w * h, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    transform_1D_to_2D(Y0, R, w, h);

    assert(MPI_Recv(Y0, w * h, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    transform_1D_to_2D(Y0, G, w, h);

    assert(MPI_Recv(Y0, w * h, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);
    transform_1D_to_2D(Y0, B, w, h);

    /* Encode block */
    assert(eps_encode_truecolor_block(R, G, B, W, H, w, h,
        x, y, resample, data, &bytes_per_block, Y_ratio,
        Cb_ratio, Cr_ratio, filter_id, mode) == EPS_OK);

    /* Send encoded data to the MASTER node */
    assert(MPI_Send(&bytes_per_block, 1, MPI_INT,
        0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);
    assert(MPI_Send(data, bytes_per_block, MPI_UNSIGNED_CHAR,
        0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    /* Free memory buffers */
    free(Y0);

    eps_free_2D((void **) R, w, h);
    eps_free_2D((void **) G, w, h);
    eps_free_2D((void **) B, w, h);

    free(data);
}

/* Decode GS block */
static void cmd_mpi_decode_gs() {
    unsigned char *Y0;
    unsigned char **Y;

    unsigned char *data;
    int data_size;

    eps_block_header hdr;
    MPI_Status status;

    /* Receive encoded data size */
    assert(MPI_Recv(&data_size, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    data = (unsigned char *) xmalloc(data_size);

    /* Receive encoded data */
    assert(MPI_Recv(data, data_size, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    /* Parse and check header */
    assert(eps_read_block_header(data, data_size, &hdr) == EPS_OK);

    /* Image buffers */
    Y = (unsigned char **) eps_malloc_2D(hdr.hdr_data.gs.w, hdr.hdr_data.gs.h,
        sizeof(unsigned char));
    Y0 = (unsigned char *) xmalloc(hdr.hdr_data.gs.w * hdr.hdr_data.gs.h);

    /* Decode data */
    assert(eps_decode_grayscale_block(Y, data, &hdr) == EPS_OK);

    /* Transform data */
    transform_2D_to_1D(Y, Y0, hdr.hdr_data.gs.w, hdr.hdr_data.gs.h);

    /* Send decoded data to the MASTER node */
    assert(MPI_Send(Y0, hdr.hdr_data.gs.w * hdr.hdr_data.gs.h,
        MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    /* Free memory buffers */
    free(Y0);
    eps_free_2D((void **) Y, hdr.hdr_data.gs.w, hdr.hdr_data.gs.h);
    free(data);
}

/* Decode TC block */
static void cmd_mpi_decode_tc() {
    unsigned char *Y0;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;

    unsigned char *data;
    int data_size;

    eps_block_header hdr;
    MPI_Status status;

    /* Receive encoded data size */
    assert(MPI_Recv(&data_size, 1, MPI_INT, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    data = (unsigned char *) xmalloc(data_size);

    /* Receive encoded data */
    assert(MPI_Recv(data, data_size, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE,
        MPI_ANY_TAG, MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

    /* Parse and check header */
    assert(eps_read_block_header(data, data_size, &hdr) == EPS_OK);

    /* Image buffers */
    R = (unsigned char **) eps_malloc_2D(hdr.hdr_data.tc.w, hdr.hdr_data.tc.h,
        sizeof(unsigned char));
    G = (unsigned char **) eps_malloc_2D(hdr.hdr_data.tc.w, hdr.hdr_data.tc.h,
        sizeof(unsigned char));
    B = (unsigned char **) eps_malloc_2D(hdr.hdr_data.tc.w, hdr.hdr_data.tc.h,
        sizeof(unsigned char));

    Y0 = (unsigned char *) xmalloc(hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);

    /* Decode data */
    assert(eps_decode_truecolor_block(R, G, B, data, &hdr) == EPS_OK);

    /* Transform and send decoded data to the MASTER node */
    transform_2D_to_1D(R, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
    assert(MPI_Send(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h,
        MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    transform_2D_to_1D(G, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
    assert(MPI_Send(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h,
        MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    transform_2D_to_1D(B, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
    assert(MPI_Send(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h,
        MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD) == MPI_SUCCESS);

    /* Free memory buffers */
    free(Y0);

    eps_free_2D((void **) R, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
    eps_free_2D((void **) G, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
    eps_free_2D((void **) B, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);

    free(data);
}

/* Main SLAVE node function */
void worker_mpi_node(int rank)
{
    MPI_Status status;
    int cmd;

    /* Process incoming requests */
    while (1) {
        assert(MPI_Recv(&cmd, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
            MPI_COMM_WORLD, (MPI_Status *) &status) == MPI_SUCCESS);

        switch (cmd) {
            case CMD_MPI_ENCODE_GS:
            {
                cmd_mpi_encode_gs();
                break;
            }
            case CMD_MPI_ENCODE_TC:
            {
                cmd_mpi_encode_tc();
                break;
            }
            case CMD_MPI_DECODE_GS:
            {
                cmd_mpi_decode_gs();
                break;
            }
            case CMD_MPI_DECODE_TC:
            {
                cmd_mpi_decode_tc();
                break;
            }
            case CMD_MPI_SHUTDOWN:
            {
                exit(0);
            }
            default:
            {
                assert(0);
            }
        }
    }
}

#endif
