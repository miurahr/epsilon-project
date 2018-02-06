package EPSILON::Utils;

#
# $Id: Utils.pm,v 1.7 2011/04/27 16:25:58 simakov Exp $
#
# EPSILON - wavelet image compression library.
# Copyright (C) 2006-2011 Alexander Simakov, <xander@entropyware.info>
#
# Helper utils for testing EPSILON
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
Readonly our $VERSION => qw($Revision: 1.7 $) [1];

use English qw( -no_match_vars );
use Carp;
use File::Spec::Functions;
use Params::Validate qw(:all);
use Cwd qw(abs_path);
use File::Basename;

#use Smart::Comments;

use base qw(Exporter);

Readonly our @EXPORT_OK => qw(
    run_epsilon
    epsilon_encoder_options_combinator
    get_all_available_filters
    get_image_path
    get_rnd_string
    write_to_file
    get_available_build_tags
    wait_for_mpi_to_cleanup
);

Readonly my $VALIDATOR_BUILD_TAG =>
    { regex => qr/\Ageneric|mpi|cluster|pthreads\z/xms };
Readonly my $MPI_CLEANUP_PAUSE => 15;

sub write_to_file {
    my $file_path    = shift;
    my $file_content = shift;

    open my $F, '>', $file_path
        or croak "Failed to open output file '$file_path': $OS_ERROR";

    print {$F} $file_content
        or croak "Cannot write to file '$file_path': $OS_ERROR";

    close $F
        or warn "Failed to close output file '$file_path': $OS_ERROR\n";

    return;
}

sub get_rnd_string {
    my $length = shift;

    ## no critic (ProhibitMagicNumbers)
    my $alpha_aref = shift || [ 'A' .. 'Z', 'a' .. 'z', 0 .. 9 ];
    ## use critic

    my @alpha = @{$alpha_aref};
    return join q{}, map { $alpha[ int rand @alpha ] } 1 .. $length;
}

sub get_image_path {
    my $image_name = shift;

    my $this_module_dir = dirname( $INC{'EPSILON/Utils.pm'} );
    my $abs_image_path  = abs_path(
        catfile( $this_module_dir, q{..}, q{..}, 'images', $image_name ) );

    return $abs_image_path;
}

sub get_available_build_tags {
    my $build_root = get_build_root();

    my @build_tags = ();
    foreach my $build_tag qw( generic pthreads cluster mpi ) {
        if ( -d catfile( $build_root, $build_tag ) ) {
            push @build_tags, $build_tag;
        }
    }
    ### available build_tags: @build_tags

    return \@build_tags;
}

sub get_build_root {
    my $build_root = $ENV{'EPSILON_TEST_BUILD_ROOT'};

    if ( !$build_root ) {
        die "Please set EPSILON_TEST_BUILD_ROOT environment variable\n";
    }

    if ( !-d $build_root ) {
        die "No such directory: '$build_root'\n";
    }

    return $build_root;
}

sub get_binary_path {
    my $build_tag = shift;

    my $build_root = get_build_root();
    my $binary_path = catfile( $build_root, $build_tag, 'bin', 'epsilon' );

    if ( !-x $binary_path ) {
        die "Cannot find '$binary_path' executable\n";
    }

    return $binary_path;
}

sub wait_for_mpi_to_cleanup {

    my $build_root = get_build_root();
    if ( -d catfile( $build_root, 'mpi' ) ) {
        sleep $MPI_CLEANUP_PAUSE;
    }

    return;
}

sub get_all_available_filters {
    my %params = validate( @_, { build_tag => $VALIDATOR_BUILD_TAG } );

    my $binary_path = get_binary_path( $params{'build_tag'} );
    my $command     = "$binary_path --list-all-fb";

    if ( $params{'build_tag'} eq 'mpi' ) {
        $command = "mpirun -np 2 $command";
    }

    my $output  = `$command`;
    my $filters = {};

LINE:
    foreach my $line ( split qr{\n}xms, $output ) {
        $line =~ s/\A[|]\s*//xms;
        $line =~ s/\s*[|]\z//xms;

        my ( $id, $name, $type ) = split qr{\s*[|]\s*}xms, $line;

        # skip over table header and footer
        next LINE if !$type;
        next LINE if $type ne 'orthogonal' and $type ne 'biorthogonal';

        $filters->{$id} = $type;
    }
    ### filters: $filters

    return $filters;
}

sub epsilon_encoder_options_combinator {
    my %params = validate( @_, { build_tag => $VALIDATOR_BUILD_TAG, } );

    my $filters
        = get_all_available_filters( build_tag => $params{'build_tag'} );

    my @option_combinations = ();

    foreach my $filter_id ( keys %{$filters} ) {
        foreach my $block_size qw( 32 64 128 256 512 1024 ) {
            foreach my $resample qw( yes no ) {
                foreach my $passes qw( one two ) {
                MODE:
                    foreach my $mode qw( normal otlpf ) {

                        # Note: 'otlpf' mode is applicable to
                        # biorthogonal filters only
                        my $filter_type = $filters->{$filter_id};
                        if (    $filter_type eq 'orthogonal'
                            and $mode eq 'otlpf' )
                        {
                            next MODE;
                        }

                        my $option_combination
                            = "--filter-id $filter_id "
                            . "--block-size $block_size "
                            . ( $resample eq 'no' ? '--no-resampling ' : q{} )
                            . ( $passes eq 'two' ? '--two-pass ' : q{} )
                            . "--mode-$mode";

                        push @option_combinations, $option_combination;
                    }
                }
            }
        }
    }
    ### option_combinations: @option_combinations

    return \@option_combinations;
}

sub run_epsilon {
    my %params = validate(
        @_,
        {   build_tag       => $VALIDATOR_BUILD_TAG,
            epsilon_options => 0,
            mpirun_options  => 0,
            file            => 1,
        }
    );

    my $binary_path = get_binary_path( $params{'build_tag'} );
    my $command = "$binary_path $params{'epsilon_options'} '$params{'file'}'";

    if ( $params{'build_tag'} eq 'mpi' ) {
        $command = "mpirun $params{'mpirun_options'} $command";
    }

    my $output = `$command 2>&1`;

    if ( $CHILD_ERROR != 0 ) {
        die "EPSILON exited with non-zero code.\n"
            . "Command: '$command'\n"
            . "Output: $output\n";
    }

    return;
}

1;
