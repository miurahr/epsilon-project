package Test::PBM::PSNR;

#
# $Id: PSNR.pm,v 1.4 2011/04/27 14:10:40 simakov Exp $
#
# EPSILON - wavelet image compression library.
# Copyright (C) 2006-2011 Alexander Simakov, <xander@entropyware.info>
#
# Test module for checking PSNR between two PGM or PPM images
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
Readonly our $VERSION => qw($Revision: 1.4 $) [1];

use English qw( -no_match_vars );
use Carp;
use Params::Validate qw(:all);

#use Smart::Comments;

use base qw(Exporter);

Readonly our @EXPORT => qw(
    is_pgm_image_psnr
    is_ppm_image_psnr
);

# E.g.: ' 38.09 dB'
Readonly my $dB_re => qr{
    \s
    (\d+(?:[.]\d*)?)
    \sdB
}xms;

my $Test = Test::Builder->new();    ## no critic (ProhibitMixedCaseVars)

sub is_pgm_image_psnr {
    my %params = validate(
        @_,
        {   reconstructed_image => 1,
            original_image      => 1,
            min_psnr            => 1,
            test_name           => 0,
        }
    );

    if ( !$params{'test_name'} ) {
        $params{'test_name'}
            = 'Check that PSNR between '
            . "'$params{'original_image'}' and '$params{'reconstructed_image'}' "
            . "is at least $params{'min_psnr'} dB";
    }

    my $got_psnr = eval {
        _get_pgm_psnr( $params{'original_image'},
            $params{'reconstructed_image'} );
    };

    if ($EVAL_ERROR) {
        print STDERR "$EVAL_ERROR\n";
        return 0;
    }

    # Images are identical, PSNR is not defined
    if ( !defined $got_psnr ) {
        $Test->ok( 1, $params{'test_name'} );

        return 1;
    }

    my $result = $Test->cmp_ok( $got_psnr, q{>=}, $params{'min_psnr'},
        $params{'test_name'} );

    if ( !$result ) {
        $Test->diag( '    actual PSNR between '
                . "'$params{'original_image'}' and '$params{'reconstructed_image'}' "
                . "is only $got_psnr dB" );

        return 0;
    }

    return 1;
}

sub is_ppm_image_psnr {
    my %params = validate(
        @_,
        {   reconstructed_image => 1,
            original_image      => 1,
            min_Y_psnr          => 1,
            min_Cb_psnr         => 1,
            min_Cr_psnr         => 1,
            test_name           => 0,
        }
    );

    if ( !$params{'test_name'} ) {
        $params{'test_name'}
            = 'Check that PSNR between '
            . "'$params{'original_image'}' and '$params{'reconstructed_image'}' "
            . 'is at least '
            . "Y: $params{'min_Y_psnr'} dB, "
            . "Cb: $params{'min_Cb_psnr'} dB, "
            . "Cr: $params{'min_Cr_psnr'} dB";
    }

    my $got_psnr_of = eval {
        _get_ppm_psnr( $params{'original_image'},
            $params{'reconstructed_image'} );
    };

    if ($EVAL_ERROR) {
        print STDERR "$EVAL_ERROR\n";
        return 0;
    }

CHANNEL:
    foreach my $channel qw(Y Cb Cr) {

        # Channels are identical, PSNR is not defined
        if ( !defined $got_psnr_of->{$channel} ) {
            next CHANNEL;
        }

        if ( $got_psnr_of->{$channel} < $params{"min_${channel}_psnr"} ) {

            # cmp_ok() will fail with nice diagnostic message
            $Test->cmp_ok(
                $got_psnr_of->{$channel},
                q{>=}, $params{"min_${channel}_psnr"},
                $params{'test_name'}
            );

            $Test->diag( "    actual $channel channel's PSNR between "
                    . "'$params{'original_image'}' and '$params{'reconstructed_image'}' "
                    . "is only $got_psnr_of->{$channel} dB" );

            return 0;
        }
    }

    $Test->ok( 1, $params{'test_name'} );

    return 1;
}

sub _get_pgm_psnr {
    my $image1 = shift;
    my $image2 = shift;

    # Sample output:
    #
    # $ pnmpsnr lena_reconstructed.pgm lena.pgm
    # pnmpsnr: PSNR between lena_reconstructed.pgm and lena.pgm: 38.09 dB
    #
    # $ pnmpsnr lena.pgm lena.pgm
    # pnmpsnr: Images lena.pgm and lena.pgm don't differ.

    my $output = _get_psnr( $image1, $image2 );

    if ( $output =~ $dB_re ) {
        return $1;
    }
    elsif ( $output =~ m{\Qdon't differ\E}xms ) {
        return undef;    ## no critic (ProhibitExplicitReturnUndef)
    }
    else {
        confess "Cannot parse pnmpsnr's output:\n" . $output;
    }
}

sub _get_ppm_psnr {      ## no critic (RequireFinalReturn)
    my $image1 = shift;
    my $image2 = shift;

    # Sample output:
    #
    # $ pnmpsnr nirvana.ppm nirvana_reconstructed.ppm
    # pnmpsnr: PSNR between nirvana.ppm and nirvana_reconstructed.ppm:
    # pnmpsnr: Y  color component: 36.68 dB
    # pnmpsnr: Cb color component: 40.66 dB
    # pnmpsnr: Cr color component: 37.42 dB
    #
    # $ pnmpsnr nirvana.ppm nirvana.ppm
    # pnmpsnr: PSNR between nirvana.ppm and nirvana.ppm:
    # pnmpsnr: Y color component doesn't differ.
    # pnmpsnr: Cb color component  doesn't differ.
    # pnmpsnr: Cr color component doesn't differ.

    my $output = _get_psnr( $image1, $image2 );

    ## no critic (ProhibitMixedCaseVars)
    my $psnr_of = {};
    foreach my $channel qw(Y Cb Cr) {
        if ( $output =~ m{ \s$channel\s .*? $dB_re$ }xms ) {
            $psnr_of->{$channel} = $1;
        }
        elsif ( $output =~ m{ \s$channel\s .*? \Qdoesn't differ\E }xms ) {
            $psnr_of->{$channel} = undef;
        }
        else {
            confess "Cannot parse pnmpsnr's output for channel '$channel':\n"
                . $output;
        }
    }

    return $psnr_of;
}

sub _get_psnr {
    my $image1 = shift;
    my $image2 = shift;

    my $output = `pnmpsnr '$image1' '$image2' 2>&1`;

    if ( $CHILD_ERROR != 0 ) {
        confess "Cannot calculate PSNR between '$image1' and '$image2': "
            . "pnmpsnr exited with non-zero code. Output:\n"
            . $output;
    }

    return $output;
}

1;
