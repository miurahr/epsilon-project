#!/usr/bin/perl

#
# $Id: verification.t,v 1.2 2011/04/27 16:25:58 simakov Exp $
#
# EPSILON - wavelet image compression library.
# Copyright (C) 2006-2011 Alexander Simakov, <xander@entropyware.info>
#
# Verification test for EPSILON. This tests tries all available builds
# (generic, pthreads, cluster, mpi) with all available wavelet filters
# in all applicable modes all possible block sizes on a list of PGM
# and PPM images. After encode/decode cycle this test compares PSNR
# with expected value - minimal common value for all option combinations.
# Compression ratio is set near to 1 (no compression) to get higher
# average PSNR for diffrerent filters, block sizes etc.
#
# This file is part of EPSILON
#
# EPSILON is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# EPSILON is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
#
# http://epsilon-project.sourceforge.net
#

use strict;
use warnings;

use Readonly;
Readonly our $VERSION => qw($Revision: 1.2 $) [1];

use English qw( -no_match_vars );
use File::Temp qw(tempdir tempfile);
use File::Spec::Functions;
use File::Basename;

#use Smart::Comments;

use FindBin qw($Bin);
FindBin::again();

use lib "$Bin/../lib";
use EPSILON::Utils qw(
    run_epsilon
    epsilon_encoder_options_combinator
    get_image_path
    get_rnd_string
    write_to_file
    get_available_build_tags
    wait_for_mpi_to_cleanup
);

use Test::More qw(no_plan);
use Test::Exception;
use Test::PBM::PSNR;

Readonly my $TMP_DIR =>
    tempdir( 'verification_XXXX', TMPDIR => 1, CLEANUP => 0 );
### TMP_DIR: $TMP_DIR

Readonly my $RND_SUFFIX_LENGTH  => 4;
Readonly my $NUMBER_OF_THREADS  => 16;
Readonly my $NUMBER_OF_MPI_CPUS => 8;
Readonly my $CLUSTER_NODE_LIST =>
    catfile( $Bin, q{..}, 'build', 'epsilon.nodes' );
Readonly my $MPI_MACHINE_FILE =>
    catfile( $Bin, q{..}, 'build', 'machines.MPICH' );

Readonly my %TEST_IMAGES => (
    'lena.pgm' => 53,

    'nirvana.ppm' => {
        min_Y_psnr  => 50,
        min_Cb_psnr => 44,
        min_Cr_psnr => 42,
    },
);

# Set to 0 if you want to check reconstructed files visually
Readonly my $CLEANUP_RECONSTRUCTED_FILES => 1;

sub common_case_tests {
    my $build_tags = shift;

    foreach my $build_tag ( @{$build_tags} ) {
        my $option_combinations
            = epsilon_encoder_options_combinator( build_tag => $build_tag );

        foreach my $image_ext ( keys %TEST_IMAGES ) {
            foreach my $option_combination ( @{$option_combinations} ) {

                # Set minimal compression ratio to get hightest PSNR possible
                my $extra_options
                    = "--ratio 1.001 --output-dir '$TMP_DIR' --quiet";
                my $epsilon_encode_options
                    = "$option_combination $extra_options";
                my $mpirun_encode_options = q{};

                # Increase number of threads for multi-threaded EPSILON build
                if ( $build_tag eq 'pthreads' ) {
                    $epsilon_encode_options
                        .= " --threads $NUMBER_OF_THREADS";
                }

                # Speclify list of nodes for EPSILON cluster
                if ( $build_tag eq 'cluster' ) {
                    $epsilon_encode_options
                        .= " --node-list $CLUSTER_NODE_LIST";
                }

                # Speclify machines file and number of CPUs MPI EPSILON
                if ( $build_tag eq 'mpi' ) {
                    $mpirun_encode_options
                        = "-machinefile $MPI_MACHINE_FILE -np $NUMBER_OF_MPI_CPUS";
                }

                # Encode file
                lives_ok {
                    run_epsilon(
                        build_tag       => $build_tag,
                        epsilon_options => $epsilon_encode_options,
                        mpirun_options  => $mpirun_encode_options,
                        file            => get_image_path($image_ext),
                    );
                }
                "[$build_tag] Encode '$image_ext' with epsilon options: "
                    . "'$epsilon_encode_options'"
                    . (
                    $mpirun_encode_options
                    ? ", mpirun options: '$mpirun_encode_options'"
                    : q{}
                    );

                my ( $image, undef, $ext )
                    = fileparse( $image_ext, qr/[.](?:pgm|ppm)/xms );
                $ext =~ s/\A[.]//xms;    # remove leading dot

                my $reconstructed_image
                    = $image
                    . '_reconstructed_'
                    . get_rnd_string($RND_SUFFIX_LENGTH);

                # Rename encoded file: add random suffix
                rename catfile( $TMP_DIR, "$image.psi" ),
                    catfile( $TMP_DIR, "$reconstructed_image.psi" );

                my $epsilon_decode_options = '--decode-file --quiet';
                my $mpirun_decode_options  = q{};

                if ( $build_tag eq 'pthreads' ) {
                    $epsilon_decode_options
                        .= " --threads $NUMBER_OF_THREADS";
                }

                if ( $build_tag eq 'cluster' ) {
                    $epsilon_decode_options
                        .= " --node-list $CLUSTER_NODE_LIST";
                }

                if ( $build_tag eq 'mpi' ) {
                    $mpirun_decode_options
                        = "-machinefile $MPI_MACHINE_FILE -np $NUMBER_OF_MPI_CPUS";
                }

                # Decode file
                lives_ok {
                    run_epsilon(
                        build_tag       => $build_tag,
                        epsilon_options => $epsilon_decode_options,
                        mpirun_options  => $mpirun_decode_options,
                        file =>
                            catfile( $TMP_DIR, "$reconstructed_image.psi" ),
                    );
                }
                "[$build_tag] Decode '$reconstructed_image.psi' with epsilon options: "
                    . "'$epsilon_decode_options'"
                    . (
                    $mpirun_decode_options
                    ? ", mpirun options: '$mpirun_decode_options'"
                    : q{}
                    );

                # Check PSNR
                my $result;
                if ( $ext eq 'pgm' ) {
                    $result = is_pgm_image_psnr(
                        original_image => get_image_path($image_ext),
                        reconstructed_image =>
                            catfile( $TMP_DIR, "$reconstructed_image.$ext" ),
                        min_psnr => $TEST_IMAGES{$image_ext},
                    );
                }
                else {
                    $result = is_ppm_image_psnr(
                        original_image => get_image_path($image_ext),
                        reconstructed_image =>
                            catfile( $TMP_DIR, "$reconstructed_image.$ext" ),
                        %{ $TEST_IMAGES{$image_ext} },
                    );
                }

                if ($result) {

                    # PSNR is ok, unlink temporary files
                    unlink catfile( $TMP_DIR, "$reconstructed_image.psi" );
                    if ($CLEANUP_RECONSTRUCTED_FILES) {
                        unlink catfile( $TMP_DIR,
                            "$reconstructed_image.$ext" );
                    }
                }
                else {

                    # PSNR is not ok, dump compression options, do not
                    # delete files
                    write_to_file(
                        catfile( $TMP_DIR, "$reconstructed_image.dump" ),
                        <<"END_OPTIONS");
build tag:              $build_tag
epsilon encode options: $epsilon_encode_options
mpirun encode options:  $mpirun_encode_options
epsilon decode options: $epsilon_decode_options
mpirun decode options:  $mpirun_decode_options
END_OPTIONS
                }
            }
        }
    }

    return;
}

sub run_tests {
    my $build_tags = get_available_build_tags();

    if ( @{$build_tags} == 0 ) {
        die "Please prepare at least one EPSILON build\n";
    }

    common_case_tests($build_tags);

    return;
}

run_tests();

END {
    wait_for_mpi_to_cleanup();

    # Removes empty dir only
    rmdir $TMP_DIR;
}
