/*
 * $Id: cmd_encode_file.c,v 1.66 2010/03/19 22:57:29 simakov Exp $
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

#ifdef ENABLE_PTHREADS
# include <pthread.h>
#endif

#ifdef ENABLE_CLUSTER
# include <sys/socket.h>
# include <sys/types.h>
# include <signal.h>
# include <cmd_start_node.h>
#endif

#ifdef ENABLE_MPI
# include <mpi.h>
# include <worker_mpi_node.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <options.h>
#include <misc.h>
#include <epsilon.h>
#include <pbm.h>
#include <psi.h>
#include <cmd_encode_file.h>
#include <cmd_truncate_file.h>

/* Check that file have .pbm or .ppm extension */
static int check_pbm_ext(char *file)
{
    char *ext;

    if (strlen(file) < 5) {
        return 0;
    }

    ext = strrchr(file, '.');

    if ((ext == NULL) || (strlen(ext) != 4)) {
        return 0;
    }

    if ((strcmp(ext, ".pgm") == 0) || (strcmp(ext, ".ppm") == 0)) {
        return 1;
    } else {
        return 0;
    }
}

/* Replace source file extension with .psi */
static void replace_pbm_to_psi(char *file)
{
    char *ext = strrchr(file, '.');
    snprintf(ext, 5, ".psi");
}

#ifdef ENABLE_MPI

/* Encode one file using MPI engine */
static void encode_file_mpi(char *filter_id, int block_size, int mode,
                            double ratio, int two_pass, int n_threads,
                            int Y_ratio, int Cb_ratio, int Cr_ratio,
                            int resample, int halt_on_errors,
                            int quiet, char *output_dir, char *file,
                            int current, int total)
{
    /* Text buffers for file names */
    char pbm_file[MAX_PATH];
    char psi_file[MAX_PATH];

    /* Text buffers to render encoding progress */
    char progress_buf[MAX_LINE];
    char timer_buf[MAX_TIMER_LINE];

    /* PBM and PSI image structures */
    pbm_image pbm;
    psi_image psi;

    /* MPI stuff */
    MPI_ctx *ctx;
    MPI_Request *req;
    MPI_Status *status;
    int *ready_indices;
    int n_ready;

    /* Input buffers */
    unsigned char **Y;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;

    /* Block calculations */
    int x_blocks, y_blocks, n_blocks;
    int done_blocks, load_blocks;
    int bytes_per_block;

    /* Image and block parameters */
    int W, H;
    int w, h;
    int x, y;

    /* Universe size */
    int size;

    /* Misc */
    int clear_len;
    int rc;

    /* Start time */
    time_t start_time = time(NULL);

    /* Prepare input and output file names */
    snprintf(pbm_file, sizeof(pbm_file), "%s", file);

    /* Save encoded file in the output_dir if set */
    if (output_dir) {
        char *pos = strrchr(file, DIR_SEPARATOR);

        snprintf(psi_file, sizeof(psi_file), "%s%c%s",
            strcmp(output_dir, "/") ? output_dir : "", DIR_SEPARATOR,
            pos ? ++pos : file);
    } else {
        snprintf(psi_file, sizeof(psi_file), "%s", file);
    }

    if (!check_pbm_ext(pbm_file)) {
        printf("Unsupported file format: %s\n", pbm_file);
        EXIT_OR_RETURN(halt_on_errors);
    }

    replace_pbm_to_psi(psi_file);

    /* Open input file */
    if ((rc = pbm_open(pbm_file, &pbm)) != PBM_OK) {
        switch (rc) {
            case PBM_FORMAT_ERROR:
            {
                pbm_close(&pbm);
                printf("Malformed file: %s\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            case PBM_SYSTEM_ERROR:
            {
                printf("Cannot open file %s: %m\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Create output file */
    if ((rc = psi_create(psi_file, &psi)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                pbm_close(&pbm);
                printf("Cannot create file %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Allocate input buffers */
    if (pbm.type == PBM_TYPE_PGM)  {
        Y = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));
    } else {
        R = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));

        G = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));

        B = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));
    }

    /* Get image width and height */
    W = pbm.width;
    H = pbm.height;

    /* Compute number of blocks */
    if (W % block_size) {
        x_blocks =  W / block_size + 1;
    } else {
        x_blocks =  W / block_size;
    }

    if (H % block_size) {
        y_blocks =  H / block_size + 1;
    } else {
        y_blocks =  H / block_size;
    }

    /* Compute number of bytes per block */
    n_blocks = x_blocks * y_blocks;
    bytes_per_block = (int) ((double) (pbm.hdr_size + pbm.data_size) /
        ((two_pass == OPT_YES ? 1.0 : ratio) * n_blocks));

    /* Clip buffer size if needed */
    if (pbm.type == PBM_TYPE_PGM)  {
        bytes_per_block = MAX(bytes_per_block, EPS_MIN_GRAYSCALE_BUF + 1);
    } else {
        bytes_per_block = MAX(bytes_per_block, EPS_MIN_TRUECOLOR_BUF + 1);
    }

    /* Reset counters */
    done_blocks = load_blocks = clear_len = 0;

    /* Initialize progress report */
    if (quiet != OPT_YES) {
        snprintf(progress_buf, sizeof(progress_buf),
            "Encoding file (%d of %d): %s - 0.00%% done in 0.00 seconds",
            current + 1, total, pbm_file);

        clear_len = strlen(progress_buf);
        printf("%s\r", progress_buf);
        fflush(stdout);
    }

    /* Get universe size (without MASTER node) */
    assert(MPI_Comm_size(MPI_COMM_WORLD, &size) == MPI_SUCCESS);
    size--;

    /* Encoding context */
    ctx = alloc_MPI_ctx(block_size, block_size, bytes_per_block, size, pbm.type);

    /* Async request handle */
    req = alloc_MPI_req(size);

    /* Array of request status */
    status = (MPI_Status *) xmalloc(sizeof(MPI_Status) * size);

    /* Array of indices of completed async requests */
    ready_indices = (int *) xmalloc(sizeof(int) * size);

    x = y = 0;

    /* Main encoding loop */
    while (done_blocks < n_blocks) {
        int i, j;

        /* Fill empty slots with a data */
        for (i = 0; i < size && load_blocks < n_blocks; i++) {
            /* Skip over non-empty slots */
            if (!ctx[i].is_free) {
                continue;
            }

            /* Block width */
            if (x + block_size > W) {
                w = W - x;
            } else {
                w = block_size;
            }

            /* Block height */
            if (y + block_size > H) {
                h = H - y;
            } else {
                h = block_size;
            }

            /* Fill context */
            ctx[i].w = w;
            ctx[i].h = h;
            ctx[i].x = x;
            ctx[i].y = y;

            if (pbm.type == PBM_TYPE_PGM) {
                int cmd = CMD_MPI_ENCODE_GS;
                MPI_Request dummy;

                /* Read and transform next input block */
                assert(pbm_read_pgm(&pbm, Y, x, y, w, h) == PBM_OK);
                transform_2D_to_1D(Y, ctx[i].Y0, w, h);

                /* Send data to the SLAVE node (async) */

                /* Command */
                assert(MPI_Isend(&cmd, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Image width */
                assert(MPI_Isend(&W, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Image height */
                assert(MPI_Isend(&H, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Block width */
                assert(MPI_Isend(&ctx[i].w, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Block height */
                assert(MPI_Isend(&ctx[i].h, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* X coord */
                assert(MPI_Isend(&ctx[i].x, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Y coord */
                assert(MPI_Isend(&ctx[i].y, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Maximum bytes-per-block value */
                assert(MPI_Isend(&bytes_per_block, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Wavelet filter ID */
                assert(MPI_Isend(filter_id, strlen(filter_id) + 1,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoding mode */
                assert(MPI_Isend(&mode, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Image data */
                assert(MPI_Isend(ctx[i].Y0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Receive answer from SLAVE node (async) */

                /* Data size */
                assert(MPI_Irecv(&ctx[i].data_size, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoded data (save async request handle in req[]) */
                assert(MPI_Irecv(ctx[i].data, bytes_per_block, MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &req[i]) == MPI_SUCCESS);
            } else {
                int cmd = CMD_MPI_ENCODE_TC;
                MPI_Request dummy;

                /* Read and transform next input block */
                assert(pbm_read_ppm(&pbm, R, G, B, x, y, w, h) == PBM_OK);

                transform_2D_to_1D(R, ctx[i].R0, w, h);
                transform_2D_to_1D(G, ctx[i].G0, w, h);
                transform_2D_to_1D(B, ctx[i].B0, w, h);

                /* Send data to the SLAVE node (async) */

                /* Command */
                assert(MPI_Isend(&cmd, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Image width */
                assert(MPI_Isend(&W, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Image height */
                assert(MPI_Isend(&H, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Block width */
                assert(MPI_Isend(&ctx[i].w, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Block height */
                assert(MPI_Isend(&ctx[i].h, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* X coord */
                assert(MPI_Isend(&ctx[i].x, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Y coord */
                assert(MPI_Isend(&ctx[i].y, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Resampling */
                assert(MPI_Isend(&resample, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Maximum bytes-per-block value */
                assert(MPI_Isend(&bytes_per_block, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Y ratio */
                assert(MPI_Isend(&Y_ratio, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Cb ratio */
                assert(MPI_Isend(&Cb_ratio, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Cr ratio */
                assert(MPI_Isend(&Cr_ratio, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Wavelet filter ID */
                assert(MPI_Isend(filter_id, strlen(filter_id) + 1,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoding mode */
                assert(MPI_Isend(&mode, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* R image channel */
                assert(MPI_Isend(ctx[i].R0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* G image channel */
                assert(MPI_Isend(ctx[i].G0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* B image channel */
                assert(MPI_Isend(ctx[i].B0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Receive answer from SLAVE node (async) */

                /* Data size */
                assert(MPI_Irecv(&ctx[i].data_size, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoded data (save async request handle in req[]) */
                assert(MPI_Irecv(ctx[i].data, bytes_per_block, MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &req[i]) == MPI_SUCCESS);
            }

            /* Mark this slot as used */
            ctx[i].is_free = 0;

            /* Proceed to next block */
            x += block_size;

            if (x >= W) {
                x = 0;
                y += block_size;
            }

            /* Try to load next block */
            load_blocks++;
        }

        /* Wait for some SLAVE nodes */
        assert(MPI_Waitsome(size, req, &n_ready, ready_indices, status) == MPI_SUCCESS);

        /* Save encoded blocks to the disk */
        for (j = 0; j < n_ready; j++) {
            assert(psi_write_next_block(&psi, ctx[ready_indices[j]].data,
                ctx[ready_indices[j]].data_size) == PSI_OK);

            /* Mark this slot as empty */
            ctx[ready_indices[j]].is_free = 1;
            done_blocks++;
        }

        /* Update progress indicator */
        if (quiet != OPT_YES) {
            time_t cur_time = time(NULL);

            /* Format text string */
            snprintf(progress_buf, sizeof(progress_buf),
                "Encoding file (%d of %d): %s - %.2f%% done in %s",
                current + 1, total, pbm_file,
                (100.0 * done_blocks / n_blocks),
                format_time((int)(cur_time - start_time),
                timer_buf, sizeof(timer_buf)));

            /* Clear old string */
            print_blank_line(clear_len);
            clear_len = strlen(progress_buf);

            /* Print next progress report and flush stdout */
            printf("%s\r", progress_buf);
            fflush(stdout);
        }
    }

    /* Free MPI stuff */
    free_MPI_ctx(ctx, size, pbm.type);
    free_MPI_req(req);
    free(status);
    free(ready_indices);

    /* Free input buffers */
    if (pbm.type == PBM_TYPE_PGM)  {
        eps_free_2D((void **) Y, block_size, block_size);
    } else {
        eps_free_2D((void **) R, block_size, block_size);
        eps_free_2D((void **) G, block_size, block_size);
        eps_free_2D((void **) B, block_size, block_size);
    }

    /* Close files */
    pbm_close(&pbm);
    psi_close(&psi);

    printf("%s", QUIET);

    /* Optimize file */
    if (two_pass == OPT_YES) {
        off_t original_size = pbm.hdr_size + pbm.data_size;
        off_t desired_size = original_size / ratio;
        off_t encoded_size;
        double truncation_ratio;

        if ((rc = get_file_size(psi_file, &encoded_size)) != PBM_OK) {
            printf("Cannot get size of %s: %m\n", psi_file);
            EXIT_OR_RETURN(halt_on_errors);
        }

        truncation_ratio = (double) encoded_size / (double) desired_size;
        truncation_ratio = MAX(truncation_ratio, 1.0);

        truncate_file(truncation_ratio, halt_on_errors, quiet,
            NULL, psi_file, current, total, OPTIMIZE_MSG);
    }
}

#else

/* Encode subset of blocks */
static void *encode_blocks(void *arg) {
    /* All arguments are packed into this structure */
    encode_ctx *ctx = (encode_ctx *) arg;

    /* Text buffers to render encoding progress */
    char progress_buf[MAX_LINE];
    char timer_buf[MAX_TIMER_LINE];

    /* Input buffers */
    unsigned char **Y;

    unsigned char **R;
    unsigned char **G;
    unsigned char **B;

#ifdef ENABLE_CLUSTER
    unsigned char *Y0;
#endif

    /* Output buffer */
    unsigned char *buf;
    int buf_size;

    int x, y;
    int w, h;

#ifdef ENABLE_PTHREADS
    /* Print mutex */
    static pthread_mutex_t p_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Read mutex */
    static pthread_mutex_t r_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Write mutex */
    static pthread_mutex_t w_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Stop mutex */
    static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef ENABLE_CLUSTER
    int sock_fd;
    int action = ctx->pbm->type == PBM_TYPE_PGM ?
        ACTION_ENCODE_GS : ACTION_ENCODE_TC;

    char ip_addr[INET_ADDRSTRLEN];
    int port;
#endif

    /* Handy shortcuts */
    int block_size = ctx->block_size;
    int quiet = ctx->quiet;
    int W = ctx->W;
    int H = ctx->H;

    /* Error flag */
    int error_flag = 0;

    int rc;
    int i;

    /* Allocate input buffers */
    if (ctx->pbm->type == PBM_TYPE_PGM)  {
        Y = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));
    } else {
        R = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));

        G = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));

        B = (unsigned char **) eps_malloc_2D(block_size, block_size,
            sizeof(unsigned char));
    }

#ifdef ENABLE_CLUSTER
    Y0 = (unsigned char *) xmalloc(block_size * block_size *
        sizeof(unsigned char));
#endif

    /* Allocate output buffer */
    buf = (unsigned char *) eps_xmalloc(ctx->bytes_per_block *
        sizeof(unsigned char));

#ifdef ENABLE_CLUSTER
    /* Get IP address and port number */
    inet_ntop(AF_INET, &ctx->node->sin_addr, ip_addr, INET_ADDRSTRLEN);
    port = ntohs(ctx->node->sin_port);

    /* Create socket */
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOCK(p_lock);
        printf("%sCannot create socket for %s port %d: %m\n",
            QUIET, ip_addr, port);
        UNLOCK(p_lock);

        /* Stop this thread and ask to stop other threads */
        error_flag = 1;
        goto error;
    }

    /* Connect to the cluster node */
    if (connect(sock_fd, (CSA *) ctx->node, sizeof(struct sockaddr_in)) != 0) {
        LOCK(p_lock);
        printf("%sCannot connect to %s port %d: %m\n",
            QUIET, ip_addr, port);
        UNLOCK(p_lock);

        /* Stop this thread and ask to stop other threads */
        error_flag = 1;
        goto error;
    }

    /* Do not halt on `Broken pipe' error */
    set_signal(SIGPIPE, SIG_IGN);

    /* Send parameters that are common for GS and TC images */
    SEND_VALUE_TO_SLAVE(action);
    SEND_VALUE_TO_SLAVE(W);
    SEND_VALUE_TO_SLAVE(H);
    SEND_VALUE_TO_SLAVE(block_size);
    SEND_VALUE_TO_SLAVE(ctx->bytes_per_block);
    SEND_VALUE_TO_SLAVE(ctx->mode);
    {
        int len = strlen(ctx->filter_id);
        SEND_VALUE_TO_SLAVE(len);
        SEND_BUF_TO_SLAVE(ctx->filter_id, len);
    }

    /* Send parameters that are specific for TC images only */
    if (action == ACTION_ENCODE_TC) {
        SEND_VALUE_TO_SLAVE(ctx->resample);
        SEND_VALUE_TO_SLAVE(ctx->Y_ratio);
        SEND_VALUE_TO_SLAVE(ctx->Cb_ratio);
        SEND_VALUE_TO_SLAVE(ctx->Cr_ratio);
    }
#endif

    /* Process all blocks */
    for (i = y = 0; y < H; y += block_size) {
        for (x = 0; x < W; x += block_size, i++) {
#ifdef ENABLE_PTHREADS
            /* Check stop flag */
            LOCK(s_lock);
            error_flag = *ctx->stop_flag;
            UNLOCK(s_lock);

            /* Stop the thread */
            if (error_flag) {
                goto error;
            }
#endif
            /* Skip over blocks not assigned to this thread */
            if ((i % ctx->n_threads) != ctx->thread_idx) {
                continue;
            }

            /* Block width */
            if (x + block_size > W) {
                w = W - x;
            } else {
                w = block_size;
            }

            /* Block height */
            if (y + block_size > H) {
                h = H - y;
            } else {
                h = block_size;
            }

#ifdef ENABLE_CLUSTER
            SEND_VALUE_TO_SLAVE(x);
            SEND_VALUE_TO_SLAVE(y);
            SEND_VALUE_TO_SLAVE(w);
            SEND_VALUE_TO_SLAVE(h);
#endif

            /* Output buffer size (not including marker) */
            buf_size = ctx->bytes_per_block - 1;

            if (ctx->pbm->type == PBM_TYPE_PGM) {
                /* Read next block */
                LOCK(r_lock);
                rc = pbm_read_pgm(ctx->pbm, Y, x, y, w, h);
                UNLOCK(r_lock);

                if (rc != PBM_OK) {
                    error_flag = 1;

                    switch (rc) {
                        case PBM_SYSTEM_ERROR:
                        {
                            LOCK(p_lock);
                            printf("%sCannot read block from %s: %m\n",
                                QUIET, ctx->pbm_file);
                            UNLOCK(p_lock);

                            goto error;
                        }
                        default:
                        {
                            assert(0);
                        }
                    }
                }

#ifdef ENABLE_CLUSTER
                /* Send raw data */
                transform_2D_to_1D(Y, Y0, w, h);
                SEND_BUF_TO_SLAVE(Y0, w * h);

                /* Receive encoded data */
                RECV_VALUE_FROM_SLAVE(&buf_size);
                RECV_BUF_FROM_SLAVE(buf, buf_size);
#else
                /* Encode block */
                rc = eps_encode_grayscale_block(Y, W, H, w, h, x, y,
                    buf, &buf_size, ctx->filter_id, ctx->mode);

                /* All function parameters are checked at the moment,
                 * so everything except EPS_OK is a logical error. */
                assert(rc == EPS_OK);
#endif

                /* Write encoded block */
                LOCK(w_lock);
                rc = psi_write_next_block(ctx->psi, buf, buf_size);
                UNLOCK(w_lock);

                if (rc != PSI_OK) {
                    error_flag = 1;

                    switch (rc) {
                        case PSI_SYSTEM_ERROR:
                        {
                            LOCK(p_lock);
                            printf("%sCannot write block to %s: %m\n",
                                QUIET, ctx->psi_file);
                            UNLOCK(p_lock);

                            goto error;
                        }
                        default:
                        {
                            assert(0);
                        }
                    }
                }
            } else {
                /* Read next block */
                LOCK(r_lock);
                rc = pbm_read_ppm(ctx->pbm, R, G, B, x, y, w, h);
                UNLOCK(r_lock);

                if (rc != PBM_OK) {
                    error_flag = 1;

                    switch (rc) {
                        case PBM_SYSTEM_ERROR:
                        {
                            LOCK(p_lock);
                            printf("%sCannot read block from %s: %m\n",
                                QUIET, ctx->pbm_file);
                            UNLOCK(p_lock);

                            goto error;
                        }
                        default:
                        {
                            assert(0);
                        }
                    }
                }

#ifdef ENABLE_CLUSTER
                /* Send raw data */
                transform_2D_to_1D(R, Y0, w, h);
                SEND_BUF_TO_SLAVE(Y0, w * h);

                transform_2D_to_1D(G, Y0, w, h);
                SEND_BUF_TO_SLAVE(Y0, w * h);

                transform_2D_to_1D(B, Y0, w, h);
                SEND_BUF_TO_SLAVE(Y0, w * h);

                /* Receive encoded data */
                RECV_VALUE_FROM_SLAVE(&buf_size);
                RECV_BUF_FROM_SLAVE(buf, buf_size);
#else
                /* Encode block */
                rc = eps_encode_truecolor_block(R, G, B, W, H, w, h,
                    x, y, ctx->resample, buf, &buf_size, (int)(ctx->Y_ratio),
                    (int)(ctx->Cb_ratio), (int)(ctx->Cr_ratio), ctx->filter_id, ctx->mode);

                /* All function parameters are checked at the moment,
                 * so everything except EPS_OK is a logical error. */
                assert(rc == EPS_OK);
#endif

                /* Write encoded block */
                LOCK(w_lock);
                rc = psi_write_next_block(ctx->psi, buf, buf_size);
                UNLOCK(w_lock);

                if (rc != PSI_OK) {
                    error_flag = 1;

                    switch (rc) {
                        case PSI_SYSTEM_ERROR:
                        {
                            LOCK(p_lock);
                            printf("%sCannot write block to %s: %m\n",
                                QUIET, ctx->psi_file);
                            UNLOCK(p_lock);

                            goto error;
                        }
                        default:
                        {
                            assert(0);
                        }
                    }
                }
            }

            /* Update progress indicator */
            if (ctx->quiet != OPT_YES) {
                time_t cur_time;
                LOCK(p_lock);
                cur_time = time(NULL);

                /* Increment counter of processed blocks */
                (*ctx->done_blocks)++;

                /* Format text string */
                snprintf(progress_buf, sizeof(progress_buf),
                    "Encoding file (%d of %d): %s - %.2f%% done in %s",
                    ctx->current + 1, ctx->total, ctx->pbm_file,
                    (100.0 * *ctx->done_blocks / ctx->n_blocks),
                    format_time((int)cur_time - ctx->start_time,
                    timer_buf, sizeof(timer_buf)));

                /* Clear old string */
                print_blank_line(*ctx->clear_len);
                *ctx->clear_len = strlen(progress_buf);

                /* Print next progress report and flush stdout */
                printf("%s\r", progress_buf);
                fflush(stdout);
                UNLOCK(p_lock);
            }
        }
    }

error:

#ifdef ENABLE_PTHREADS
    /* Ask other threads to stop */
    if (error_flag) {
        LOCK(s_lock);
        *ctx->stop_flag = 1;
        UNLOCK(s_lock);
    }
#endif

#ifdef ENABLE_CLUSTER
    /* Close connection */
    close(sock_fd);
#endif

    /* Free input buffers */
    if (ctx->pbm->type == PBM_TYPE_PGM)  {
        eps_free_2D((void **) Y, block_size, block_size);
    } else {
        eps_free_2D((void **) R, block_size, block_size);
        eps_free_2D((void **) G, block_size, block_size);
        eps_free_2D((void **) B, block_size, block_size);
    }

#ifdef ENABLE_CLUSTER
    free(Y0);
#endif

    /* Free output buffer */
    free(buf);

    /* Return 0 for success or 0 for error */
    return (void *) error_flag;
}

/* Encode one file */
static void encode_file(char *filter_id, int block_size, int mode,
                        double ratio, int two_pass, int n_threads,
                        void *cluster, int Y_ratio, int Cb_ratio,
                        int Cr_ratio, int resample, int halt_on_errors,
                        int quiet, char *output_dir, char *file,
                        int current, int total)
{
    /* Text buffers for file names */
    char pbm_file[MAX_PATH];
    char psi_file[MAX_PATH];

    /* Text buffers to render encoding progress */
    char progress_buf[MAX_LINE];

    /* PBM and PSI image structures */
    pbm_image pbm;
    psi_image psi;

#ifdef ENABLE_PTHREADS
    /* Arrays for thread CTXs and IDs */
    encode_ctx ctx[MAX_N_THREADS];
    pthread_t tid[MAX_N_THREADS];
#else
    /* Single context for thread-unaware version */
    encode_ctx ctx[1];
#endif

#ifdef ENABLE_CLUSTER
    struct sockaddr_in *nodes = (struct sockaddr_in *) cluster;
#endif

    int bytes_per_block;

    int x_blocks, y_blocks;
    int n_blocks, done_blocks;

    int clear_len;
    int stop_flag;
    int W, H;

    int rc;
    int i;

    /* Start time */
    time_t start_time = time(NULL);

    /* Prepare input and output file names */
    snprintf(pbm_file, sizeof(pbm_file), "%s", file);

    /* Save encoded file in the output_dir if set */
    if (output_dir) {
        char *pos = strrchr(file, DIR_SEPARATOR);

        snprintf(psi_file, sizeof(psi_file), "%s%c%s",
            strcmp(output_dir, "/") ? output_dir : "", DIR_SEPARATOR,
            pos ? ++pos : file);
    } else {
        snprintf(psi_file, sizeof(psi_file), "%s", file);
    }

    if (!check_pbm_ext(pbm_file)) {
        printf("Unsupported file format: %s\n", pbm_file);
        EXIT_OR_RETURN(halt_on_errors);
    }

    replace_pbm_to_psi(psi_file);

    /* Open input file */
    if ((rc = pbm_open(pbm_file, &pbm)) != PBM_OK) {
        switch (rc) {
            case PBM_FORMAT_ERROR:
            {
                pbm_close(&pbm);
                printf("Malformed file: %s\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            case PBM_SYSTEM_ERROR:
            {
                printf("Cannot open file %s: %m\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Create output file */
    if ((rc = psi_create(psi_file, &psi)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                pbm_close(&pbm);
                printf("Cannot create file %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Get image width and height */
    W = pbm.width;
    H = pbm.height;

    /* Compute number of blocks */
    if (W % block_size) {
        x_blocks =  W / block_size + 1;
    } else {
        x_blocks =  W / block_size;
    }

    if (H % block_size) {
        y_blocks =  H / block_size + 1;
    } else {
        y_blocks =  H / block_size;
    }

    /* Compute number of bytes per block */
    n_blocks = x_blocks * y_blocks;
    bytes_per_block = (int) ((double) (pbm.hdr_size + pbm.data_size) /
        ((two_pass == OPT_YES ? 1.0 : ratio) * n_blocks));

    /* Clip buffer size if needed */
    if (pbm.type == PBM_TYPE_PGM)  {
        bytes_per_block = MAX(bytes_per_block, EPS_MIN_GRAYSCALE_BUF + 1);
    } else {
        bytes_per_block = MAX(bytes_per_block, EPS_MIN_TRUECOLOR_BUF + 1);
    }

    done_blocks = clear_len = stop_flag = 0;

    /* Initialize progress report */
    if (quiet != OPT_YES) {
        snprintf(progress_buf, sizeof(progress_buf),
            "Encoding file (%d of %d): %s - 0.00%% done in 0.00 seconds",
            current + 1, total, pbm_file);

        clear_len = strlen(progress_buf);
        printf("%s\r", progress_buf);
        fflush(stdout);
    }

    /* Prepare CTXs */
    for (i = 0; i < n_threads; i++) {
        ctx[i].filter_id = filter_id;
        ctx[i].block_size = block_size;
        ctx[i].mode = mode;
        ctx[i].bytes_per_block = bytes_per_block;
        ctx[i].Y_ratio = Y_ratio;
        ctx[i].Cb_ratio = Cb_ratio;
        ctx[i].Cr_ratio = Cr_ratio;
        ctx[i].resample = resample;
        ctx[i].W = W;
        ctx[i].H = H;
        ctx[i].n_blocks = n_blocks;
        ctx[i].done_blocks = &done_blocks;
        ctx[i].n_threads = n_threads;
        ctx[i].thread_idx = i;
#ifdef ENABLE_CLUSTER
        ctx[i].node = &nodes[i];
#endif
        ctx[i].start_time = start_time;
        ctx[i].pbm_file = pbm_file;
        ctx[i].psi_file = psi_file;
        ctx[i].pbm = &pbm;
        ctx[i].psi = &psi;
        ctx[i].current = current;
        ctx[i].total = total;
        ctx[i].quiet = quiet;
        ctx[i].clear_len = &clear_len;
        ctx[i].stop_flag = &stop_flag;
    }

#ifdef ENABLE_PTHREADS
    /* Set concurrency level */
    assert(!pthread_setconcurrency(n_threads));

    /* Create threads */
    for (i = 0; i < n_threads; i++) {
        assert(!pthread_create(&tid[i], NULL, encode_blocks, (void *) &ctx[i]));
    }
#else
    /* Encode blocks */
    rc = (int) encode_blocks((void *) &ctx[0]);
#endif

#ifdef ENABLE_PTHREADS
    /* Join all threads */
    for (rc = i = 0; i < n_threads; i++) {
        void *thread_return;

        assert(!pthread_join(tid[i], &thread_return));
        rc |= (int) thread_return;
    }
#endif

    /* Close files */
    pbm_close(&pbm);
    psi_close(&psi);

    /* Check for errors */
    if (rc) {
#ifdef ENABLE_PTHREADS
        printf("%s", QUIET);
#endif
        EXIT_OR_RETURN(halt_on_errors);
    } else {
        printf("%s", QUIET);

        /* Optimize file */
        if (two_pass == OPT_YES) {
            off_t original_size = pbm.hdr_size + pbm.data_size;
            off_t desired_size = (off_t)(original_size / ratio);
            off_t encoded_size;
            double truncation_ratio;

            if ((rc = get_file_size(psi_file, &encoded_size)) != PBM_OK) {
                printf("Cannot get size of %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }

            truncation_ratio = (double) encoded_size / (double) desired_size;
            truncation_ratio = MAX(truncation_ratio, 1.0);

            truncate_file(truncation_ratio, halt_on_errors, quiet,
                          NULL, psi_file, current, total, OPTIMIZE_MSG);
        }
    }
}

#endif

/* Encode files */
void cmd_encode_file(char *filter_id, int block_size, int mode,
                     double ratio, int two_pass, int n_threads,
                     char *node_list, int Y_ratio, int Cb_ratio,
                     int Cr_ratio, int resample, int halt_on_errors,
                     int quiet, char *output_dir, char **files)
{
    int filter_type;
    int i, n;

    char timer_buf[MAX_TIMER_LINE];
    time_t total_time;

#ifdef ENABLE_CLUSTER
    struct sockaddr_in nodes[MAX_CLUSTER_NODES];

    /* Load list of cluster nodes */
    if (node_list) {
        /* File is specified explicitly */
        n_threads = load_cluster_nodes(node_list, nodes);
    } else if (file_exists(DEFAULT_NODE_LIST)) {
        /* Try file in the currect directory */
        n_threads = load_cluster_nodes(DEFAULT_NODE_LIST, nodes);
    } else {
        if (getenv("HOME")) {
            char pathname[MAX_PATH];

            /* Try file in user directory */
            snprintf(pathname, sizeof(pathname), "%s%c%s",
                getenv("HOME"), DIR_SEPARATOR, DEFAULT_NODE_LIST);

            if (file_exists(pathname)) {
                n_threads = load_cluster_nodes(pathname, nodes);
            } else {
                /* Giving up */
                printf("You should specify list of cluster nodes.\n");
                exit(1);
            }
        } else {
            /* Giving up */
            printf("You should specify list of cluster nodes.\n");
            exit(1);
        }
    }
#endif

    /* Check filter */
    if (filter_id == OPT_NA) {
        filter_id = OPT_DEF_FB;
        filter_type = OPT_DEF_FB_TYPE;
    } else {
        char **id, **next_id;
        char **type, **next_type;

        next_id = id = eps_get_fb_info(EPS_FB_ID);
        next_type = type = eps_get_fb_info(EPS_FB_TYPE);

        for (;;) {
            if (!(*next_id) || !(*next_type)) {
                printf("Filter %s not found.\n", filter_id);
                exit(1);
            }

            if (strcmp(*next_id, filter_id) == 0) {
                if (strcmp(*next_type, "biorthogonal") == 0) {
                    filter_type = BIORTHOGONAL;
                } else {
                    filter_type = ORTHOGONAL;
                }

                break;
            }

            *next_id++;
            *next_type++;
        }

        eps_free_fb_info(id);
        eps_free_fb_info(type);
    }

    /* Check filter orthogonality type and encoding mode */
    if (filter_type == BIORTHOGONAL) {
        if ((mode == OPT_NA) || (mode == OPT_MODE_OTLPF)) {
            mode = EPS_MODE_OTLPF;
        } else {
            mode = EPS_MODE_NORMAL;
        }
    } else {
        if ((mode == OPT_NA) || (mode == OPT_MODE_NORMAL)) {
            mode = EPS_MODE_NORMAL;
        } else {
            printf("OTLPF mode can be used with biorthogonal filters only.\n");
            exit(1);
        }
    }

    /* Check block size */
    if (block_size == OPT_NA) {
        block_size = OPT_DEF_BLOCK + (mode == EPS_MODE_OTLPF);
    } else {
        if (block_size < EPS_MIN_BLOCK_SIZE) {
            printf("Block size too small.\n");
            exit(1);
        }

        if (block_size > EPS_MAX_BLOCK_SIZE) {
            printf("Block size too large.\n");
            exit(1);
        }

        if (!power_of_two(block_size)) {
            printf("Block size must be power of two.\n");
            exit(1);
        }

        block_size += (mode == EPS_MODE_OTLPF);
    }

    /* Check compression ratio */
    if (ratio == OPT_NA) {
        ratio = OPT_DEF_RATIO;
    } else {
        if (ratio <= 1.0) {
            printf("Compression ratio must be greater than 1.0\n");
            exit(1);
        }
    }

    /* Check the number of threads */
    if ((n_threads < 1) || (n_threads > MAX_N_THREADS)) {
        printf("Incorrect value for the number of threads.\n");
        exit(1);
    }

    /* Check bit-budget distribution */
    if ((Y_ratio  == OPT_NA) ||
        (Cb_ratio == OPT_NA) ||
        (Cr_ratio == OPT_NA))
    {
        Y_ratio = EPS_Y_RT;
        Cb_ratio = EPS_Cb_RT;
        Cr_ratio = EPS_Cr_RT;
    } else {
        if ((Y_ratio < EPS_MIN_RT) || (Y_ratio > EPS_MAX_RT)) {
            printf("Incorrect value for the Y channel ratio.\n");
            exit(1);
        }

        if ((Cb_ratio < EPS_MIN_RT) || (Cb_ratio > EPS_MAX_RT)) {
            printf("Incorrect value for the Cb channel ratio.\n");
            exit(1);
        }

        if ((Cr_ratio < EPS_MIN_RT) || (Cr_ratio > EPS_MAX_RT)) {
            printf("Incorrect value for the Cr channel ratio.\n");
            exit(1);
        }

        if (Y_ratio + Cb_ratio + Cr_ratio != 100) {
            printf("The sum of Y, Cb and Cr must give 100%%\n");
            exit(1);
        }
    }

    /* Check resampling mode */
    if (resample == OPT_YES) {
        resample = EPS_RESAMPLE_420;
    } else {
        resample = EPS_RESAMPLE_444;
    }

    n = get_number_of_files(files);

    if (!n) {
        printf("No input files.\n");
        exit(1);
    }

    total_time = time(NULL);

    /* Process all input files */
    for (i = 0; i < n; i++) {
        void *cluster = NULL;
#ifdef ENABLE_CLUSTER
        cluster = nodes;
#endif

#ifdef ENABLE_MPI
        encode_file_mpi(filter_id, block_size, mode, ratio, two_pass,
                        n_threads, Y_ratio, Cb_ratio, Cr_ratio,
                        resample, halt_on_errors, quiet, output_dir,
                        files[i], i, n);
#else
        encode_file(filter_id, block_size, mode, ratio, two_pass,
                    n_threads, cluster, Y_ratio, Cb_ratio, Cr_ratio,
                    resample, halt_on_errors, quiet, output_dir,
                    files[i], i, n);
#endif
    }

    total_time = time(NULL) - total_time;

    if (n > 1 && quiet == OPT_NO) {
        printf("Total %d file(s) done in %s\n",
            n, format_time(total_time, timer_buf, sizeof(timer_buf)));
    }
}
