#!/usr/bin/perl

#
# $Id: quick.t,v 1.2 2011/04/28 10:15:47 simakov Exp $
#
# EPSILON - wavelet image compression library.
# Copyright (C) 2006-2011 Alexander Simakov, <xander@entropyware.info>
#
# Quick verification test for generic EPSILON build.
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
    get_image_path
    get_rnd_string
);

use Test::More;
use Test::Exception;
use Test::PBM::PSNR;

Readonly my $TMP_DIR => tempdir( 'quick_XXXX', TMPDIR => 1, CLEANUP => 0 );
### TMP_DIR: $TMP_DIR

Readonly my $RND_SUFFIX_LENGTH => 4;
Readonly my $BUILD_TAG         => 'generic';

Readonly my $CHECKS_PER_IMAGE => 3;
Readonly my %TEST_IMAGES      => (
    'gray_dot.pgm'            => 60.00,
    'horizontal_gradient.pgm' => 49.50,
    'vertical_gradient.pgm'   => 49.80,
    'red_dot.ppm'             => {
        min_Y_psnr  => 58.60,
        min_Cb_psnr => 54.10,
        min_Cr_psnr => 63.50,
    },
    'horizontal_rainbow.ppm' => {
        min_Y_psnr  => 49.00,
        min_Cb_psnr => 40.00,
        min_Cr_psnr => 36.00,
    },

    'vertical_rainbow.ppm' => {
        min_Y_psnr  => 52.40,
        min_Cb_psnr => 31.50,
        min_Cr_psnr => 40.10,
    },
    'lena.pgm'    => 53.20,
    'nirvana.ppm' => {
        min_Y_psnr  => 55.20,
        min_Cb_psnr => 44.50,
        min_Cr_psnr => 42.90,
    },
);

# Set to 0 if you want to check reconstructed files visually
Readonly my $CLEANUP_RECONSTRUCTED_FILES => 1;

sub set_test_plan {
    plan tests => $CHECKS_PER_IMAGE * keys %TEST_IMAGES;

    return;
}

sub quick_test {
    foreach my $image_ext ( keys %TEST_IMAGES ) {

        # Set minimal compression ratio to get hightest PSNR possible
        my $epsilon_encode_options
            = "--ratio 1.001 --output-dir '$TMP_DIR' --quiet";

        # Encode file
        lives_ok {
            run_epsilon(
                build_tag       => $BUILD_TAG,
                epsilon_options => $epsilon_encode_options,
                file            => get_image_path($image_ext),
            );
        }
        "[$BUILD_TAG] Encode '$image_ext' with epsilon options: "
            . "'$epsilon_encode_options'";

        my ( $image, undef, $ext )
            = fileparse( $image_ext, qr/[.](?:pgm|ppm)/xms );
        $ext =~ s/\A[.]//xms;    # remove leading dot

        my $reconstructed_image
            = $image . '_reconstructed_' . get_rnd_string($RND_SUFFIX_LENGTH);

        # Rename encoded file: add random suffix
        rename catfile( $TMP_DIR, "$image.psi" ),
            catfile( $TMP_DIR, "$reconstructed_image.psi" );

        my $epsilon_decode_options = '--decode-file --quiet';

        # Decode file
        lives_ok {
            run_epsilon(
                build_tag       => $BUILD_TAG,
                epsilon_options => $epsilon_decode_options,
                file => catfile( $TMP_DIR, "$reconstructed_image.psi" ),
            );
        }
        "[$BUILD_TAG] Decode '$reconstructed_image.psi' with epsilon options: "
            . "'$epsilon_decode_options'";

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
                unlink catfile( $TMP_DIR, "$reconstructed_image.$ext" );
            }
        }
    }

    return;
}

sub run_tests {
    set_test_plan();
    quick_test();

    return;
}

run_tests();

END {

    # Removes empty dir only
    rmdir $TMP_DIR;
}
