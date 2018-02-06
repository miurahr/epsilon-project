/*
 * $Id: cmd_truncate_file.c,v 1.6 2010/03/19 22:57:29 simakov Exp $
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
#include <string.h>
#include <assert.h>
#include <time.h>
#include <options.h>
#include <misc.h>
#include <epsilon.h>
#include <psi.h>
#include <cmd_truncate_file.h>
#include <cmd_decode_file.h>

/* Replace source file extension with .tmp */
static void replace_psi_to_tmp(char *file)
{
    char *ext = strrchr(file, '.');
    snprintf(ext, 5, ".tmp");
}

/* Truncate one file */
void truncate_file(double ratio, int halt_on_errors,
                   int quiet, char *output_dir, char *file,
                   int current, int total, char *msg)
{
    char psi_in_file[MAX_PATH];
    char psi_out_file[MAX_PATH];
    char progress_buf[MAX_LINE];
    char timer_buf[MAX_TIMER_LINE];

    eps_block_header hdr;

    psi_image psi_in;
    psi_image psi_out;
    pbm_image pbm_tmp;

    unsigned char *buf_in;
    unsigned char *buf_out;
    int buf_size;

    int max_block_w, max_block_h;
    int x_blocks, y_blocks;
    int n_blocks, done_blocks;

    int W, H;

    int error_flag = 0;
    int clear_len;
    int rc;

    time_t start_time = time(NULL);
    time_t cur_time;

    /* Check input file extension */
    if (!check_psi_ext(file)) {
        printf("Unsupported file format: %s\n", file);
        EXIT_OR_RETURN(halt_on_errors);
    }

    /* Prepare input and output file names */
    snprintf(psi_in_file, sizeof(psi_in_file), "%s", file);

    /* Save truncated file in the output_dir if set */
    if (output_dir) {
        char *pos = strrchr(file, DIR_SEPARATOR);

        snprintf(psi_out_file, sizeof(psi_out_file), "%s%c%s",
            strcmp(output_dir, "/") ? output_dir : "", DIR_SEPARATOR,
            pos ? ++pos : file);
    } else {
        snprintf(psi_out_file, sizeof(psi_out_file), "%s", file);
        replace_psi_to_tmp(psi_out_file);
    }

    /* Open input file */
    if ((rc = psi_open(psi_in_file, &psi_in)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                printf("Cannot open file %s: %m\n", psi_in_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Try to guess target file type: PGM or PPM */
    if ((rc = psi_guess_pbm_type(&psi_in, &pbm_tmp)) != PSI_OK) {
        switch (rc) {
            case PSI_GUESS_ERROR:
            {
                psi_close(&psi_in);
                printf("Cannot guess file type: %s\n", psi_in_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            case PSI_SYSTEM_ERROR:
            {
                psi_close(&psi_in);
                printf("Cannot guess type of %s: %m\n", psi_in_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    /* Create output file */
    if ((rc = psi_create(psi_out_file, &psi_out)) != PSI_OK) {
        switch (rc) {
            case PSI_SYSTEM_ERROR:
            {
                psi_close(&psi_in);
                printf("Cannot create file %s: %m\n", psi_out_file);
                EXIT_OR_RETURN(halt_on_errors);
            }
            default:
            {
                assert(0);
            }
        }
    }

    if (pbm_tmp.type == PBM_TYPE_PGM) {
        buf_size = EPS_MAX_GRAYSCALE_BUF;
    } else {
        buf_size = EPS_MAX_TRUECOLOR_BUF;
    }

    /* Allocate input and output buffers */
    buf_in = (unsigned char *) eps_xmalloc(buf_size *
        sizeof(unsigned char));

    buf_out = (unsigned char *) eps_xmalloc(buf_size *
        sizeof(unsigned char));

    /* Overall image width and heigth */
    W = pbm_tmp.width;
    H = pbm_tmp.height;

    /* Larget block width and height */
    max_block_w = psi_in.max_block_w;
    max_block_h = psi_in.max_block_h;

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
            "%s file (%d of %d): %s - 0.00%% done in 0.00 seconds\r",
            msg, current + 1, total, psi_in_file);

        clear_len = strlen(progress_buf);
        printf("%s\r", progress_buf);
        fflush(stdout);
    }

    /* Process all blocks */
    for (done_blocks = 1; done_blocks <= n_blocks; done_blocks++) {
        int real_buf_size = buf_size;
        int truncate_size;

        /* Read next input block */
        if ((rc = psi_read_next_block(&psi_in, buf_in, &real_buf_size)) != PSI_OK) {
            error_flag = 1;

            switch (rc) {
                case PSI_SYSTEM_ERROR:
                {
                    printf("%sCannot read block from %s: %m\n", QUIET, psi_in_file);
                    goto error;
                }
                case PSI_EOF:
                {
                    printf("%sUnexpected end of file: %s\n", QUIET, psi_in_file);
                    goto error;
                }
                default:
                {
                    assert(0);
                }
            }
        }

        /* Parse and check block header */
        rc = eps_read_block_header(buf_in, real_buf_size, &hdr);

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

        truncate_size = (int)(real_buf_size / ratio);

        if (truncate_size < MAX(EPS_MIN_GRAYSCALE_BUF, EPS_MIN_TRUECOLOR_BUF)) {
            truncate_size = MAX(EPS_MIN_GRAYSCALE_BUF, EPS_MIN_TRUECOLOR_BUF);
        }

        /* Truncate block */
        rc = eps_truncate_block(buf_in, buf_out, &hdr, &truncate_size);
        assert(rc == EPS_OK);

        /* Write truncated block */
        rc = psi_write_next_block(&psi_out, buf_out, truncate_size);

        if (rc != PSI_OK) {
            error_flag = 1;

            switch (rc) {
                case PSI_SYSTEM_ERROR:
                {
                    printf("%sCannot write block to %s: %m\n", QUIET, psi_out_file);
                    goto error;
                }
                default:
                {
                    assert(0);
                }
            }
        }

        /* Update progress indicator */
        if (quiet != OPT_YES) {
            cur_time = time(NULL);

            snprintf(progress_buf, sizeof(progress_buf),
                "%s file (%d of %d): %s - %.2f%% done in %s", msg,
                current + 1, total, psi_in_file, (100.0 * done_blocks / n_blocks),
                format_time((int)(cur_time - start_time), timer_buf, sizeof(timer_buf)));

            print_blank_line(clear_len);
            clear_len = strlen(progress_buf);
            printf("%s\r", progress_buf);
            fflush(stdout);
        }
    }

error:
    /* Free input and output buffers */
    free(buf_in);
    free(buf_out);

    /* Close files */
    psi_close(&psi_in);
    psi_close(&psi_out);

    if (!output_dir) {
        if (rename(psi_out_file, psi_in_file) == -1) {
            printf("%sCannot move %s to %s: %m\n", QUIET,
                psi_in_file, psi_out_file);

            EXIT_OR_RETURN(halt_on_errors);
        }
    }

    /* Check error_flag */
    if (error_flag) {
        EXIT_OR_RETURN(halt_on_errors);
    } else {
        printf("%s", QUIET);
    }
}

/* Truncate files */
void cmd_truncate_file(double ratio, int halt_on_errors, int quiet,
                       char *output_dir, char **files,  char *msg)
{
    int i, n;

    char timer_buf[MAX_TIMER_LINE];
    time_t total_time;

    /* Check truncation ratio */
    if (ratio == OPT_NA) {
        printf("You should specify truncation ratio\n");
        exit(1);
    } else if (ratio <= 1.0) {
        printf("Compression ratio must be greater than 1.0\n");
        exit(1);
    }

    n = get_number_of_files(files);

    if (!n) {
        printf("No input files\n");
        exit(1);
    }

    total_time = time(NULL);

    /* Process all input files */
    for (i = 0; i < n; i++) {
        truncate_file(ratio, halt_on_errors, quiet,
                      output_dir, files[i], i, n, msg);
    }

    total_time = time(NULL) - total_time;

    if (n > 1 && quiet == OPT_NO) {
        printf("Total %d file(s) done in %s\n",
            n, format_time((int)total_time, timer_buf, sizeof(timer_buf)));
    }
}
