/*
 * $Id: epsilon.c,v 1.201 2010/04/05 05:01:04 simakov Exp $
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
#include <popt.h>

#include <options.h>
#include <misc.h>
#include <cmd_encode_file.h>
#include <cmd_decode_file.h>
#include <cmd_truncate_file.h>
#include <cmd_list_all_fb.h>
#include <cmd_version.h>

#ifdef ENABLE_CLUSTER
# include <cmd_start_node.h>
#endif

#ifdef ENABLE_MPI
# include <mpi.h>
# include <worker_mpi_node.h>
#endif

void parse_options(int argc, char **argv)
{
    int opt_command             = OPT_NA;
    char *opt_filter_id         = OPT_NA;
    int opt_block_size          = OPT_NA;
    int opt_mode                = OPT_NA;
    double opt_ratio            = OPT_NA;
    int opt_Y_ratio             = OPT_NA;
    int opt_Cb_ratio            = OPT_NA;
    int opt_Cr_ratio            = OPT_NA;
    int opt_n_threads           = DEF_N_THREADS;
    int opt_resample            = OPT_YES;
    int opt_two_pass            = OPT_NO;
#ifdef ENABLE_MPI
    int opt_halt_on_errors      = OPT_YES;
#else
    int opt_halt_on_errors      = OPT_NO;
#endif
    int opt_quiet               = OPT_NO;
    int opt_ignore_hdr_crc      = OPT_NO;
    int opt_ignore_data_crc     = OPT_NO;
    int opt_ignore_format_err   = OPT_NO;
    char **opt_files            = OPT_NA;
    char *opt_output_dir        = OPT_NA;
#ifdef ENABLE_CLUSTER
    int opt_port                = OPT_NA;
#endif
    char *opt_node_list         = OPT_NA;

    int rc;

    /* Main command */
    struct poptOption cmd_options[] = {
        { "encode-file", 'e', POPT_ARG_VAL, &opt_command,
           OPT_CMD_ENCODE_FILE, "Encode specified file(s)", NULL },
        { "decode-file", 'd', POPT_ARG_VAL, &opt_command,
           OPT_CMD_DECODE_FILE, "Decode specified file(s)", NULL },
        { "truncate-file", 't', POPT_ARG_VAL, &opt_command,
           OPT_CMD_TRUNCATE_FILE, "Truncate specified file(s)", NULL },
#ifdef ENABLE_CLUSTER
        { "start-node", 's', POPT_ARG_VAL, &opt_command,
           OPT_CMD_START_NODE, "Start cluster node", NULL },
#endif
        { "list-all-fb", 'a', POPT_ARG_VAL, &opt_command,
          OPT_CMD_LIST_ALL_FB, "List all available filterbanks", NULL },
        { "version", 'V', POPT_ARG_VAL, &opt_command,
          OPT_CMD_VERSION, "Print program version", NULL },
        POPT_TABLEEND
    };

    /* Encode options */
    struct poptOption encode_file_options[] = {
        { "filter-id", 'f', POPT_ARG_STRING, &opt_filter_id,
          0, "Wavelet filterbank ID", "ID" },
        { "block-size", 'b', POPT_ARG_INT, &opt_block_size,
          0, "Block size: 32,64,128,256,512 or 1024", "VALUE" },
        { "mode-normal", 'n', POPT_ARG_VAL, &opt_mode,
          OPT_MODE_NORMAL, "Normal processing mode", NULL },
        { "mode-otlpf", 'o', POPT_ARG_VAL, &opt_mode,
          OPT_MODE_OTLPF, "OTLPF processing mode", NULL },
        { "ratio", 'r', POPT_ARG_DOUBLE, &opt_ratio,
          0, "Desired compression ratio", "VALUE" },
        { "two-pass", '2', POPT_ARG_VAL, &opt_two_pass,
          OPT_YES, "Two-pass encoding mode", NULL },
#if defined(ENABLE_PTHREADS) && !defined(ENABLE_CLUSTER)
        { "threads", 'T', POPT_ARG_INT, &opt_n_threads,
          0, "Number of threads", "VALUE" },
#endif
#ifdef ENABLE_CLUSTER
        { "node-list", 'N', POPT_ARG_STRING, &opt_node_list,
          0, "List of cluster nodes", "FILE" },
#endif
        { "Y-ratio", '\0', POPT_ARG_INT, &opt_Y_ratio,
          0, "Bit-budget percent for the Y channel", "VALUE" },
        { "Cb-ratio", '\0', POPT_ARG_INT, &opt_Cb_ratio,
          0, "Bit-budget percent for the Cb channel", "VALUE" },
        { "Cr-ratio", '\0', POPT_ARG_INT, &opt_Cr_ratio,
          0, "Bit-budget percent for the Cr channel", "VALUE" },
        { "no-resampling", '\0', POPT_ARG_VAL, &opt_resample,
          OPT_NO, "Omit image resampling", NULL },
        POPT_TABLEEND
    };

    /* Decode file options */
    struct poptOption decode_file_options[] = {
#if defined(ENABLE_PTHREADS) && !defined(ENABLE_CLUSTER)
        { "threads", 'T', POPT_ARG_INT, &opt_n_threads,
          0, "Number of threads", "VALUE" },
#endif
#ifdef ENABLE_CLUSTER
        { "node-list", 'N', POPT_ARG_STRING, &opt_node_list,
          0, "List of cluster nodes", "FILE" },
#endif
        { "ignore-hdr-crc", '\0', POPT_ARG_VAL, &opt_ignore_hdr_crc,
          OPT_YES, "Ignore header CRC errors", NULL },
        { "ignore-data-crc", '\0', POPT_ARG_VAL, &opt_ignore_data_crc,
          OPT_YES, "Ignore data CRC errors", NULL },
        { "ignore-format-err", '\0', POPT_ARG_VAL, &opt_ignore_format_err,
          OPT_YES, "Ignore malformed blocks", NULL },
        POPT_TABLEEND
    };

    /* Truncate file options */
    struct poptOption truncate_file_options[] = {
        { "ratio", 'r', POPT_ARG_DOUBLE, &opt_ratio,
          0, "Desired truncation ratio", "VALUE" },
        POPT_TABLEEND
    };

#ifdef ENABLE_CLUSTER
    /* Start node options */
    struct poptOption start_node_options[] = {
        { "port", 'P', POPT_ARG_INT, &opt_port,
          0, "Port number", "VALUE" },
        POPT_TABLEEND
    };
#endif

    /* Common stuff */
    struct poptOption common_options[] = {
#ifndef ENABLE_MPI
        { "halt-on-errors", 'H', POPT_ARG_VAL, &opt_halt_on_errors,
          OPT_YES , "Halt on errors", NULL },
#endif
        { "quiet", 'q', POPT_ARG_VAL, &opt_quiet,
          OPT_YES , "Be quiet", NULL },
        { "output-dir", 'O', POPT_ARG_STRING, &opt_output_dir,
          0, "Output directory", "DIR" },
        POPT_TABLEEND
    };

    /* Put everything together */
    struct poptOption options[] = {
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &cmd_options,
          0, "Commands:", NULL },
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &encode_file_options,
          0, "Options to use with `--encode-file' command:", NULL },
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &decode_file_options,
          0, "Options to use with `--decode-file' command:", NULL },
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &truncate_file_options,
          0, "Options to use with `--truncate-file' command:", NULL },
#ifdef ENABLE_CLUSTER
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &start_node_options,
          0, "Options to use with `--start-node' command:", NULL },
#endif
        { NULL, '\0', POPT_ARG_INCLUDE_TABLE, &common_options,
          0, "Common options:", NULL },
        POPT_AUTOHELP
        POPT_TABLEEND
    };

    poptContext opt_ctx;

    /* Parse options */
    opt_ctx = poptGetContext("epsilon", argc, (const char **) argv,
        options, 0);

    poptSetOtherOptionHelp(opt_ctx, "COMMAND [OPTIONS] FILES...");

    rc = poptGetNextOpt(opt_ctx);

    if (rc < -1) {
        printf("%s: %s\n",
            poptBadOption(opt_ctx, POPT_BADOPTION_NOALIAS),
            poptStrerror(rc));
        exit(1);
    }

    /* Input or output files */
    opt_files = (char **) poptGetArgs(opt_ctx);

    /* Dispatch command */
    switch (opt_command) {
        case OPT_NA:
        case OPT_CMD_ENCODE_FILE:
        {
            cmd_encode_file(opt_filter_id, opt_block_size, opt_mode,
                            opt_ratio, opt_two_pass, opt_n_threads,
                            opt_node_list, opt_Y_ratio, opt_Cb_ratio,
                            opt_Cr_ratio, opt_resample, opt_halt_on_errors,
                            opt_quiet, opt_output_dir, opt_files);
            break;
        }
        case OPT_CMD_DECODE_FILE:
        {
            cmd_decode_file(opt_n_threads, opt_node_list, opt_halt_on_errors,
                            opt_quiet, opt_ignore_hdr_crc, opt_ignore_data_crc,
                            opt_ignore_format_err, opt_output_dir, opt_files);
            break;
        }
        case OPT_CMD_TRUNCATE_FILE:
        {
            cmd_truncate_file(opt_ratio, opt_halt_on_errors, opt_quiet,
                              opt_output_dir, opt_files, TRUNCATE_MSG);
            break;
        }
#ifdef ENABLE_CLUSTER
        case OPT_CMD_START_NODE:
        {

            cmd_start_node(opt_port);
            break;
        }
#endif
        case OPT_CMD_LIST_ALL_FB:
        {
            cmd_list_all_fb();
            break;
        }
        case OPT_CMD_VERSION:
        {
            cmd_version();
            break;
        }
    }

    poptFreeContext(opt_ctx);
    exit(0);
}

int main(int argc, char **argv)
{
#ifdef ENABLE_MPI
    int rank, size;

    /* Initialize MPI engine */
    assert(MPI_Init(&argc, &argv) == MPI_SUCCESS);
    assert(!atexit((void *) &shutdown_nodes));

    /* Get my rank and universe size */
    assert(MPI_Comm_rank(MPI_COMM_WORLD, &rank) == MPI_SUCCESS);
    assert(MPI_Comm_size(MPI_COMM_WORLD, &size) == MPI_SUCCESS);

    /* Check for errors */
    if (size < 2) {
        printf("EPSILON/MPI needs at least 2 running nodes\n");
        exit(1);
    }

    /* Launch worker MPI node */
    if (rank) {
        worker_mpi_node(rank);
        exit(0);
    }
#endif

    parse_options(argc, argv);
    exit(0);
}
