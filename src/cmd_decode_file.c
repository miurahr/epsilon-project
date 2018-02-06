/*
 * $Id: cmd_decode_file.c,v 1.37 2010/03/19 22:57:29 simakov Exp $
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
#include <cmd_decode_file.h>

/* Check that file have .psi extension */
int check_psi_ext(char *file)
{
    char *ext;

    if (strlen(file) < 5) {
        return 0;
    }

    ext = strrchr(file, '.');

    if ((ext == NULL) || (strlen(ext) != 4)) {
        return 0;
    }

    if (strcmp(ext, ".psi") == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* Replace source file extension with .pgm or .ppm */
static void replace_psi_to_pbm(char *file, int pbm_type)
{
    char *ext = strrchr(file, '.');

    if (pbm_type == PBM_TYPE_PGM) {
        snprintf(ext, 5, ".pgm");
    } else {
        snprintf(ext, 5, ".ppm");
    }
}

/* Decode subset of blocks */
static void *decode_blocks(void *arg) {
    /* All arguments are packed into this structure */
    decode_ctx *ctx = (decode_ctx *) arg;

    /* Text buffers to render decoding progress */
    char progress_buf[MAX_LINE];
    char timer_buf[MAX_TIMER_LINE];

#ifdef ENABLE_PTHREADS
    /* Print mutex */
    static pthread_mutex_t p_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Read mutex */
    static pthread_mutex_t r_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Write mutex */
    static pthread_mutex_t w_lock = PTHREAD_MUTEX_INITIALIZER;
    /* Stop mutex */
    static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
    /* EOF-check mutex */
    static pthread_mutex_t e_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

    /* Input buffer */
    unsigned char *buf;

    /* Output buffers */
    unsigned char **Y;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;

#ifdef ENABLE_CLUSTER
    unsigned char *Y0;

    int sock_fd;
    int action = ctx->pbm->type == PBM_TYPE_PGM ?
        ACTION_DECODE_GS : ACTION_DECODE_TC;

    char ip_addr[INET_ADDRSTRLEN];
    int port;
#endif

    /* Handy shortcuts */
    int max_block_w = ctx->psi->max_block_w;
    int max_block_h = ctx->psi->max_block_h;
#ifdef ENABLE_CLUSTER
    int block_size = MAX(max_block_w, max_block_h);
#endif
    int W = ctx->W;
    int H = ctx->H;
    int quiet = ctx->quiet;

    /* Error flag */
    int error_flag = 0;

    /* Allocate input buffer */
    buf = (unsigned char *) eps_xmalloc(ctx->buf_size *
        sizeof(unsigned char));

    /* Allocate output buffers */
    if (ctx->pbm->type == PBM_TYPE_PGM) {
        Y = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));
    } else {
        R = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));

        G = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));

        B = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));
    }

#ifdef ENABLE_CLUSTER
    Y0 = (unsigned char *) xmalloc(block_size * block_size *
        sizeof(unsigned char));

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
    SEND_VALUE_TO_SLAVE(ctx->buf_size);
    SEND_VALUE_TO_SLAVE(block_size);
#endif

    /* Process blocks */
    while (1) {
        eps_block_header hdr;
        int real_buf_size = ctx->buf_size;
        int rc;

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

        /* Are there any unprocessed blocks? */
        LOCK(e_lock);
        if (*ctx->done_blocks == ctx->n_blocks) {
            UNLOCK(e_lock);
            break;
        } else {
            (*ctx->done_blocks)++;
        }
        UNLOCK(e_lock);

        /* Read next input block */
        LOCK(r_lock);
        rc = psi_read_next_block(ctx->psi, buf, &real_buf_size);
        UNLOCK(r_lock);

        if (rc != PSI_OK) {
            error_flag = 1;

            switch (rc) {
                case PSI_SYSTEM_ERROR:
                {
                    LOCK(p_lock);
                    printf("%sCannot read block from %s: %m\n",
                        QUIET, ctx->psi_file);
                    UNLOCK(p_lock);

                    goto error;
                }
                case PSI_EOF:
                {
                    LOCK(p_lock);
                    printf("%sUnexpected end of file: %s\n",
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

        /* Parse and check block header */
        rc = eps_read_block_header(buf, real_buf_size, &hdr);

        if (rc != EPS_OK) {
            switch (rc) {
                case EPS_FORMAT_ERROR:
                {
                    if (ctx->ignore_format_err == OPT_NO) {
                        error_flag = 1;

                        LOCK(p_lock);
                        printf("%sMalformed block: %s\n",
                            QUIET, ctx->psi_file);
                        UNLOCK(p_lock);

                        goto error;
                    }

                    /* Skip over malformed block */
                    continue;
                }
                default:
                {
                    assert(0);
                }
            }
        }

        /* Check header CRC flag */
        if ((hdr.chk_flag == EPS_BAD_CRC) && (ctx->ignore_hdr_crc == OPT_NO)) {
            error_flag = 1;

            LOCK(p_lock);
            printf("%sIncorrect header CRC: %s\n", QUIET, ctx->psi_file);
            UNLOCK(p_lock);

            goto error;
        }

        /* Check data CRC flag */
        if ((hdr.crc_flag == EPS_BAD_CRC) && (ctx->ignore_data_crc == OPT_NO)) {
            error_flag = 1;

            LOCK(p_lock);
            printf("%sIncorrect data CRC: %s\n", QUIET, ctx->psi_file);
            UNLOCK(p_lock);

            goto error;
        }

        if (ctx->pbm->type == PBM_TYPE_PGM) {
            /* Skip over broken blocks */
            if ((hdr.hdr_data.gs.W != W) || (hdr.hdr_data.gs.H != H)) {
                continue;
            }

#ifdef ENABLE_CLUSTER
            /* Send encoded data */
            SEND_VALUE_TO_SLAVE(real_buf_size);
            SEND_BUF_TO_SLAVE(buf, real_buf_size);

            /* Receive raw data */
            RECV_BUF_FROM_SLAVE(Y0, hdr.hdr_data.gs.w * hdr.hdr_data.gs.h);
            transform_1D_to_2D(Y0, Y, hdr.hdr_data.gs.w, hdr.hdr_data.gs.h);
#else
            /* All function parameters are checked at the moment,
             * so everything except EPS_OK is a logical error. */
            rc = eps_decode_grayscale_block(Y, buf, &hdr);
            assert(rc == EPS_OK);
#endif

            LOCK(w_lock);
            rc = pbm_write_pgm(ctx->pbm, Y,
                               hdr.hdr_data.gs.x, hdr.hdr_data.gs.y,
                               hdr.hdr_data.gs.w, hdr.hdr_data.gs.h);
            UNLOCK(w_lock);

            if (rc != PBM_OK) {
                error_flag = 1;

                switch (rc) {
                    case PBM_SYSTEM_ERROR:
                    {
                        LOCK(p_lock);
                        printf("%sCannot write block to %s: %m\n", QUIET, ctx->pbm_file);
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
            /* Skip over broken blocks */
            if ((hdr.hdr_data.tc.W != W) || (hdr.hdr_data.tc.H != H)) {
                continue;
            }

#ifdef ENABLE_CLUSTER
            /* Send encoded data */
            SEND_VALUE_TO_SLAVE(real_buf_size);
            SEND_BUF_TO_SLAVE(buf, real_buf_size);

            /* Receive raw data */
            RECV_BUF_FROM_SLAVE(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
            transform_1D_to_2D(Y0, R, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);

            RECV_BUF_FROM_SLAVE(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
            transform_1D_to_2D(Y0, G, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);

            RECV_BUF_FROM_SLAVE(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
            transform_1D_to_2D(Y0, B, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
#else
            /* Decode block */
            rc = eps_decode_truecolor_block(R, G, B, buf, &hdr);

            if (rc != EPS_OK) {
                switch (rc) {
                    case EPS_FORMAT_ERROR:
                    {
                        /* Skip over broken blocks */
                        continue;
                    }
                    default:
                    {
                        assert(0);
                    }
                }
            }
#endif

            /* Write encoded block */
            LOCK(w_lock);
            rc = pbm_write_ppm(ctx->pbm, R, G, B,
                               hdr.hdr_data.tc.x, hdr.hdr_data.tc.y,
                               hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
            UNLOCK(w_lock);

            if (rc != PBM_OK) {
                error_flag = 1;

                switch (rc) {
                    case PBM_SYSTEM_ERROR:
                    {
                        LOCK(p_lock);
                        printf("%sCannot write block to %s: %m\n", QUIET, ctx->pbm_file);
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
        if (quiet != OPT_YES) {
            time_t cur_time;
            LOCK(p_lock);
            cur_time = time(NULL);

            snprintf(progress_buf, sizeof(progress_buf),
                "Decoding file (%d of %d): %s - %.2f%% done in %s",
                ctx->current + 1, ctx->total, ctx->psi_file,
                (100.0 * *ctx->done_blocks / ctx->n_blocks),
                format_time((int)(cur_time - ctx->start_time),
                timer_buf, sizeof(timer_buf)));

            print_blank_line(*ctx->clear_len);
            *ctx->clear_len = strlen(progress_buf);
            printf("%s\r", progress_buf);
            fflush(stdout);
            UNLOCK(p_lock);
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

    /* Free input buffer */
    free(buf);

    /* Free output buffers */
    if (ctx->pbm->type == PBM_TYPE_PGM) {
        eps_free_2D((void **) Y, max_block_w, max_block_h);
    } else {
        eps_free_2D((void **) R, max_block_w, max_block_h);
        eps_free_2D((void **) G, max_block_w, max_block_h);
        eps_free_2D((void **) B, max_block_w, max_block_h);
    }

#ifdef ENABLE_CLUSTER
    free(Y0);
#endif

    /* Return 0 for success or 0 for error */
    return (void *) error_flag;
}

#ifdef ENABLE_MPI
static void decode_file_mpi(int n_threads, int halt_on_errors, int quiet,
                            int ignore_hdr_crc, int ignore_data_crc,
                            int ignore_format_err, char *output_dir,
                            char *file, int current, int total)
{
    /* Text buffers for file names */
    char psi_file[MAX_PATH];
    char pbm_file[MAX_PATH];

    /* Text buffer to render decoding progress */
    char progress_buf[MAX_LINE];
    char timer_buf[MAX_TIMER_LINE];

    /* PSI and PBM image structures */
    psi_image psi;
    pbm_image pbm;

    /* MPI stuff */
    MPI_ctx *ctx;
    MPI_Request *req;
    MPI_Status *status;
    int *ready_indices;
    int n_ready;

    /* Output buffers */
    unsigned char **Y;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;

    /* Input buffer size */
    int buf_size;

    /* Miscellaneous block stuff */
    int max_block_w, max_block_h;
    int x_blocks, y_blocks, n_blocks;
    int load_blocks, done_blocks;
    int block_size;

    /* Universe size */
    int size;

    int clear_len;
    int W, H;

    int rc;
    int i;

    /* Start time */
    time_t start_time = time(NULL);

    /* Prepare input and output file names */
    snprintf(psi_file, sizeof(psi_file), "%s", file);

    /* Save decoded file in the output_dir if set */
    if (output_dir) {
        char *pos = strrchr(file, DIR_SEPARATOR);

        snprintf(pbm_file, sizeof(pbm_file), "%s%c%s",
            strcmp(output_dir, "/") ? output_dir : "", DIR_SEPARATOR,
            pos ? ++pos : file);
    } else {
        snprintf(pbm_file, sizeof(pbm_file), "%s", file);
    }

    if (!check_psi_ext(psi_file)) {
        printf("Unsupported file format: %s\n", psi_file);
        EXIT_OR_RETURN(halt_on_errors);
    }

    /* Open input file */
    if ((rc = psi_open(psi_file, &psi)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                printf("Cannot open file %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Try to guess target file type: PGM or PPM */
    if ((rc = psi_guess_pbm_type(&psi, &pbm)) != PSI_OK) {
        switch (rc) {
            case PSI_GUESS_ERROR:
            {
                psi_close(&psi);
                printf("Cannot guess file type: %s\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            case PSI_SYSTEM_ERROR:
            {
                psi_close(&psi);
                printf("Cannot guess type of %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    replace_psi_to_pbm(pbm_file, pbm.type);

    /* Create output file */
    if ((rc = pbm_create(pbm_file, &pbm)) != PBM_OK) {
        switch (rc) {
            case PBM_SYSTEM_ERROR:
            {
                psi_close(&psi);
                pbm_close(&pbm);
                printf("Cannot create file %s: %m\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Estimate input buffer size */
    if (pbm.type == PBM_TYPE_PGM) {
        buf_size = EPS_MAX_GRAYSCALE_BUF;
    } else {
        buf_size = EPS_MAX_TRUECOLOR_BUF;
    }

    /* Overall image width and heigth */
    W = pbm.width;
    H = pbm.height;

    /* Larget block width and height */
    max_block_w = psi.max_block_w;
    max_block_h = psi.max_block_h;

    block_size = MAX(max_block_w, max_block_h);

    /* Allocate input buffers */
    if (pbm.type == PBM_TYPE_PGM)  {
        Y = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));
    } else {
        R = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));

        G = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));

        B = (unsigned char **) eps_malloc_2D(max_block_w, max_block_h,
            sizeof(unsigned char));
    }

    /* Compute number of blocks */
    if (W % max_block_w) {
        x_blocks =  W / max_block_w + 1;
    } else {
        x_blocks =  W / max_block_w;
    }

    if (H % max_block_h) {
        y_blocks =  H / max_block_h + 1;
    } else {
        y_blocks =  H / max_block_h;
    }

    /* Compute total number of blocks in the file */
    n_blocks = x_blocks * y_blocks;

    /* Initialize progress indicator */
    if (quiet != OPT_YES) {
        snprintf(progress_buf, sizeof(progress_buf),
            "Decoding file (%d of %d): %s - 0.00%% done in 0.00 seconds",
            current + 1, total, psi_file);

        clear_len = strlen(progress_buf);
        printf("%s\r", progress_buf);
        fflush(stdout);
    }

    /* Get universe size (without MASTER node) */
    assert(MPI_Comm_size(MPI_COMM_WORLD, &size) == MPI_SUCCESS);
    size--;

    /* Decoding context */
    ctx = alloc_MPI_ctx(max_block_w, max_block_h, buf_size, size, pbm.type);

    /* Async request handle */
    req = alloc_MPI_req(size);

    /* Array of request status */
    status = (MPI_Status *) xmalloc(sizeof(MPI_Status) * size);

    /* Array of indices of completed async requests */
    ready_indices = (int *) xmalloc(sizeof(int) * size);

    /* Reset counters */
    done_blocks = load_blocks = 0;

    /* Main decoding loop */
    while (1) {
        eps_block_header hdr;
        int i, j;
        int rc;

        /* Fill empty slots with a data */
        for (i = 0; i < size && load_blocks < n_blocks; i++) {
            /* Skip over non-empty slots */
            if (!ctx[i].is_free) {
                continue;
            }

            /* Read next input block */
            ctx[i].data_size = buf_size;
            rc = psi_read_next_block(&psi, ctx[i].data, &ctx[i].data_size);
            assert(rc == PSI_OK);

            /* Parse and check block header */
            rc = eps_read_block_header(ctx[i].data, ctx[i].data_size, &hdr);
            assert(rc == EPS_OK);

            if (pbm.type == PBM_TYPE_PGM) {
                int cmd = CMD_MPI_DECODE_GS;
                MPI_Request dummy;

                /* Fill context */
                ctx[i].w = hdr.hdr_data.gs.w;
                ctx[i].h = hdr.hdr_data.gs.h;
                ctx[i].x = hdr.hdr_data.gs.x;
                ctx[i].y = hdr.hdr_data.gs.y;

                /* Send data to the SLAVE node (async) */

                /* Command */
                assert(MPI_Isend(&cmd, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Data size */
                assert(MPI_Isend(&ctx[i].data_size, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoded data */
                assert(MPI_Isend(ctx[i].data, ctx[i].data_size,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Receive answer from SLAVE node (async) */

                /* Receive decoded data */
                assert(MPI_Irecv(ctx[i].Y0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &req[i]) == MPI_SUCCESS);
            } else {
                int cmd = CMD_MPI_DECODE_TC;
                MPI_Request dummy;

                /* Fill context */
                ctx[i].w = hdr.hdr_data.tc.w;
                ctx[i].h = hdr.hdr_data.tc.h;
                ctx[i].x = hdr.hdr_data.tc.x;
                ctx[i].y = hdr.hdr_data.tc.y;

                /* Send data to the SLAVE node (async) */

                /* Command */
                assert(MPI_Isend(&cmd, 1, MPI_INT, i + 1, 0,
                    MPI_COMM_WORLD, (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Data size */
                assert(MPI_Isend(&ctx[i].data_size, 1, MPI_INT, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Encoded data */
                assert(MPI_Isend(ctx[i].data, ctx[i].data_size,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                /* Receive answer from SLAVE node (async) */

                /* Receive decoded data */
                assert(MPI_Irecv(ctx[i].R0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                assert(MPI_Irecv(ctx[i].G0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &dummy) == MPI_SUCCESS);
                FREE_DUMMY_HANDLE();

                assert(MPI_Irecv(ctx[i].B0, ctx[i].w * ctx[i].h,
                    MPI_UNSIGNED_CHAR, i + 1, 0, MPI_COMM_WORLD,
                    (MPI_Request *) &req[i]) == MPI_SUCCESS);
            }

            /* Mark this slot as used */
            ctx[i].is_free = 0;

            /* Try to load next block */
            load_blocks++;
        }

        /* Wait for some SLAVE nodes */
        assert(MPI_Waitsome(size, req, &n_ready, ready_indices, status) == MPI_SUCCESS);

        /* Save decoded blocks to the disk */
        if (pbm.type == PBM_TYPE_PGM) {
            for (j = 0; j < n_ready; j++) {
                /* Transform block */
                transform_1D_to_2D(ctx[ready_indices[j]].Y0, Y,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);

                /* Save block */
                rc = pbm_write_pgm(&pbm, Y,
                    ctx[ready_indices[j]].x, ctx[ready_indices[j]].y,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);
                assert(rc == PBM_OK);

                /* Mark this slot as empty */
                ctx[ready_indices[j]].is_free = 1;
                done_blocks++;
            }
        } else {
            for (j = 0; j < n_ready; j++) {
                /* Transform block */
                transform_1D_to_2D(ctx[ready_indices[j]].R0, R,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);

                transform_1D_to_2D(ctx[ready_indices[j]].G0, G,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);

                transform_1D_to_2D(ctx[ready_indices[j]].B0, B,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);

                /* Save block */
                rc = pbm_write_ppm(&pbm, R, G, B,
                    ctx[ready_indices[j]].x, ctx[ready_indices[j]].y,
                    ctx[ready_indices[j]].w, ctx[ready_indices[j]].h);
                assert(rc == PBM_OK);

                /* Mark this slot as empty */
                ctx[ready_indices[j]].is_free = 1;
                done_blocks++;
            }
        }

        /* Update progress indicator */
        if (quiet != OPT_YES) {
            time_t cur_time = time(NULL);

            snprintf(progress_buf, sizeof(progress_buf),
                "Decoding file (%d of %d): %s - %.2f%% done in %s",
                current + 1, total, psi_file, (100.0 * done_blocks / n_blocks),
                format_time(cur_time - start_time, timer_buf, sizeof(timer_buf)));

            print_blank_line(clear_len);
            clear_len = strlen(progress_buf);
            printf("%s\r", progress_buf);
            fflush(stdout);
        }

        if (done_blocks == n_blocks) {
            break;
        }
    }

    printf("%s", QUIET);

    /* Close files */
    psi_close(&psi);
    pbm_close(&pbm);

    /* Free MPI stuff */
    free_MPI_ctx(ctx, size, pbm.type);
    free_MPI_req(req);
    free(status);
    free(ready_indices);

    /* Free input buffers */
    if (pbm.type == PBM_TYPE_PGM)  {
        eps_free_2D((void **) Y, max_block_w, max_block_h);
    } else {
        eps_free_2D((void **) R, max_block_w, max_block_h);
        eps_free_2D((void **) G, max_block_w, max_block_h);
        eps_free_2D((void **) B, max_block_w, max_block_h);
    }
}
#endif

/* Decode file */
static void decode_file(int n_threads, void *cluster, int halt_on_errors,
                        int quiet, int ignore_hdr_crc, int ignore_data_crc,
                        int ignore_format_err, char *output_dir,
                        char *file, int current, int total)
{
    /* Text buffers for file names */
    char psi_file[MAX_PATH];
    char pbm_file[MAX_PATH];

    /* Text buffer to render decoding progress */
    char progress_buf[MAX_LINE];

    /* PSI and PBM image structures */
    psi_image psi;
    pbm_image pbm;

#ifdef ENABLE_PTHREADS
    /* Arrays for thread CTXs and IDs */
    decode_ctx ctx[MAX_N_THREADS];
    pthread_t tid[MAX_N_THREADS];
#else
    /* Single context for thread-unaware version */
    decode_ctx ctx[1];
#endif

#ifdef ENABLE_CLUSTER
    struct sockaddr_in *nodes = (struct sockaddr_in *) cluster;
#endif

    /* Input buffer size */
    int buf_size;

    /* Miscellaneous block stuff */
    int max_block_w, max_block_h;
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
    snprintf(psi_file, sizeof(psi_file), "%s", file);

    /* Save decoded file in the output_dir if set */
    if (output_dir) {
        char *pos = strrchr(file, DIR_SEPARATOR);

        snprintf(pbm_file, sizeof(pbm_file), "%s%c%s",
            strcmp(output_dir, "/") ? output_dir : "", DIR_SEPARATOR,
            pos ? ++pos : file);
    } else {
        snprintf(pbm_file, sizeof(pbm_file), "%s", file);
    }

    if (!check_psi_ext(psi_file)) {
        printf("Unsupported file format: %s\n", psi_file);
        EXIT_OR_RETURN(halt_on_errors);
    }

    /* Open input file */
    if ((rc = psi_open(psi_file, &psi)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                printf("Cannot open file %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Try to guess target file type: PGM or PPM */
    if ((rc = psi_guess_pbm_type(&psi, &pbm)) != PSI_OK) {
        switch (rc) {
            case PSI_GUESS_ERROR:
            {
                psi_close(&psi);
                printf("Cannot guess file type: %s\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            case PSI_SYSTEM_ERROR:
            {
                psi_close(&psi);
                printf("Cannot guess type of %s: %m\n", psi_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    replace_psi_to_pbm(pbm_file, pbm.type);

    /* Create output file */
    if ((rc = pbm_create(pbm_file, &pbm)) != PBM_OK) {
        switch (rc) {
            case PBM_SYSTEM_ERROR:
            {
                psi_close(&psi);
                pbm_close(&pbm);
                printf("Cannot create file %s: %m\n", pbm_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Estimate input buffer size */
    if (pbm.type == PBM_TYPE_PGM) {
        buf_size = EPS_MAX_GRAYSCALE_BUF;
    } else {
        buf_size = EPS_MAX_TRUECOLOR_BUF;
    }

    /* Overall image width and heigth */
    W = pbm.width;
    H = pbm.height;

    /* Larget block width and height */
    max_block_w = psi.max_block_w;
    max_block_h = psi.max_block_h;

    /* Compute number of blocks */
    if (W % max_block_w) {
        x_blocks =  W / max_block_w + 1;
    } else {
        x_blocks =  W / max_block_w;
    }

    if (H % max_block_h) {
        y_blocks =  H / max_block_h + 1;
    } else {
        y_blocks =  H / max_block_h;
    }

    /* Compute total number of blocks in the file */
    n_blocks = x_blocks * y_blocks;

    /* Initialize progress indicator */
    if (quiet != OPT_YES) {
        snprintf(progress_buf, sizeof(progress_buf),
            "Decoding file (%d of %d): %s - 0.00%% done in 0.00 seconds",
            current + 1, total, psi_file);

        clear_len = strlen(progress_buf);
        printf("%s\r", progress_buf);
        fflush(stdout);
    }

    done_blocks = stop_flag = 0;

    /* Prepare CTXs */
    for (i = 0; i < n_threads; i++) {
        ctx[i].buf_size = buf_size;
        ctx[i].n_blocks = n_blocks;
        ctx[i].done_blocks = &done_blocks;
        ctx[i].clear_len = &clear_len;
        ctx[i].W = W;
        ctx[i].H = H;
        ctx[i].start_time = start_time;
        ctx[i].psi_file = psi_file;
        ctx[i].pbm_file = pbm_file;
        ctx[i].psi = &psi;
        ctx[i].pbm = &pbm;
#ifdef ENABLE_CLUSTER
        ctx[i].node = &nodes[i];
#endif
        ctx[i].current = current;
        ctx[i].total = total;
        ctx[i].ignore_hdr_crc = ignore_hdr_crc;
        ctx[i].ignore_data_crc = ignore_data_crc;
        ctx[i].ignore_format_err = ignore_format_err;
        ctx[i].quiet = quiet;
        ctx[i].stop_flag = &stop_flag;
    }

#ifdef ENABLE_PTHREADS
    /* Set concurrency level */
    assert(!pthread_setconcurrency(n_threads));

    /* Create threads */
    for (i = 0; i < n_threads; i++) {
        assert(!pthread_create(&tid[i], NULL, decode_blocks, (void *) &ctx[i]));
    }

    /* Join all threads */
    for (rc = i = 0; i < n_threads; i++) {
        void *thread_return;

        assert(!pthread_join(tid[i], &thread_return));
        rc |= (int) thread_return;
    }
#else
    /* Decode blocks */
    rc = (int) decode_blocks((void *) &ctx[0]);
#endif

    /* Close files */
    psi_close(&psi);
    pbm_close(&pbm);

    /* Check for errors */
    if (rc) {
#ifdef ENABLE_PTHREADS
        printf("%s", QUIET);
#endif
        EXIT_OR_RETURN(halt_on_errors);
    } else {
        printf("%s", QUIET);
    }
}

/* Decode files */
void cmd_decode_file(int n_threads, char *node_list, int halt_on_errors,
                     int quiet, int ignore_hdr_crc, int ignore_data_crc,
                     int ignore_format_err, char *output_dir, char **files)
{
    char timer_buf[MAX_TIMER_LINE];
    time_t total_time;

    int i, n;

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

    /* Get number of files */
    n = get_number_of_files(files);

    if (!n) {
        printf("No input files\n");
        exit(1);
    }

    /* Check the number of threads */
    if ((n_threads < 1) || (n_threads > MAX_N_THREADS)) {
        printf("Incorrect value for the number of threads.\n");
        exit(1);
    }

    total_time = time(NULL);

    /* Process all files */
    for (i = 0; i < n; i++) {
        void *cluster = NULL;
#ifdef ENABLE_CLUSTER
        cluster = nodes;
#endif

#ifdef ENABLE_MPI
        decode_file_mpi(n_threads, halt_on_errors, quiet, ignore_hdr_crc,
                    ignore_data_crc, ignore_format_err, output_dir,
                    files[i], i, n);
#else
        decode_file(n_threads, cluster, halt_on_errors, quiet, ignore_hdr_crc,
                    ignore_data_crc, ignore_format_err, output_dir,
                    files[i], i, n);
#endif
    }

    total_time = time(NULL) - total_time;

    if (n > 1 && quiet == OPT_NO) {
        printf("Total %d file(s) done in %s\n",
            n, format_time((int)total_time, timer_buf, sizeof(timer_buf)));
    }
}
