#!/usr/bin/perl -w

#
# $Id: make_filterbank.pl,v 1.10 2010/02/05 23:50:23 simakov Exp $
#
# EPSILON - wavelet image compression library.
# Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
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

#
# This program makes C-structures from XML-based filter specifications.
# Usage example: make_filterbank *.filters
#

use strict;
use warnings;
use XML::Simple;

sub Main {
    unless (@ARGV) {
        print "Usage: make_filterbank.pl <files>\n";
        exit(1);
    }

    # Process all files
    for my $file (@ARGV) {
        my $filter = parse_xml_file($file);
        check_filter($filter);
        make_filterbank($filter);
    }
}

sub parse_xml_file {
    my $file = shift @_;

    print STDERR "Parsing file $file...\n";

    my $xml = XML::Simple->new();
    my $filter = $xml->XMLin($file);

    return $filter;
}

sub check_filter {
    my $filter = shift @_;

    # Check filter id
    unless (defined($filter->{'id'})) {
        print STDERR "Filter id not specified.\n";
        exit(1);
    }

    unless ($filter->{'id'} =~ /^[a-zA-Z_]\w*$/) {
        print STDERR "Filter id must be alphanumerical and start with letter.\n";
        exit(1);
    }

    # Check filter name
    unless (defined($filter->{'name'})) {
        print STDERR "Filter name not specified.\n";
        exit(1);
    }

    # Check filter type
    unless (defined($filter->{'type'})) {
        print STDERR "Filter type not specified.\n";
        exit(1);
    }

    if ((lc($filter->{'type'}) ne 'orthogonal') and
        (lc($filter->{'type'}) ne 'biorthogonal'))
    {
        print STDERR "Filter type must be either orthogonal or biorthogonal.\n";
        exit(1);
    }

    # Make pretty comment
    if (defined($filter->{'info'})) {
        my @strings = split(/\r?\n/, $filter->{'info'});
        my $info;

        STRING: for my $s (@strings) {
            $s =~ s/^\s+//g;
            $s =~ s/\s+$//g;

            next STRING unless($s);

            unless ($info) {
                $info = "/* $s\n";
                next STRING;
            }

            $info .= " * $s\n";
        }

        chomp($info);

        $info .= " */\n";
        $filter->{'info'} = $info;
    }

    # Check primary filter
    unless (defined($filter->{'primary'}->{'content'})) {
        print STDERR "Primary filter not specified.\n";
        exit(1);
    }

    unless (defined($filter->{'primary'}->{'causality'})) {
        print STDERR "Primary filter causality not specified.\n";
        exit(1);
    }

    if ((lc($filter->{'primary'}->{'causality'}) ne 'causal') and
        (lc($filter->{'primary'}->{'causality'}) ne 'anticausal') and
        (lc($filter->{'primary'}->{'causality'}) ne 'symmetric_whole') and
        (lc($filter->{'primary'}->{'causality'}) ne 'symmetric_half'))
    {
        print STDERR "Acceptable values for filter causality: ".
                     "causal, anticausal, symmetric_whole and symmetric_half.\n";
        exit(1);
    }

    if ((lc($filter->{'type'}) eq 'orthogonal') and
        (lc($filter->{'primary'}->{'causality'}) ne 'anticausal'))
    {
        print STDERR "Orthogonal filter must be anticausal.\n";
        exit(1);
    }

    if ((lc($filter->{'type'}) eq 'biorthogonal') and
        (lc($filter->{'primary'}->{'causality'}) !~ '^symmetric'))
    {
        print STDERR "Biorthogonal filter must be either ".
                     "symmetric_whole or symmetric_half.\n";
        exit(1);
    }

    # Even-length biorthogonal filters are not supported yet.
    if (lc($filter->{'primary'}->{'causality'}) eq 'symmetric_half') {
        print STDERR "Biorthogonal symmetric_half filters are not supported yet.\n";
        exit(1);
    }

    my $primary_coeffs = $filter->{'primary'}->{'content'};

    $primary_coeffs =~ s/^\s+//g;
    $primary_coeffs =~ s/\s+$//g;
    $primary_coeffs =~ s/\s+/ /g;

    $filter->{'primary'}->{'content'} = $primary_coeffs;

    # For biorthogonal filters check the dual filter as well
    if (lc($filter->{'type'}) eq 'biorthogonal') {
        unless (defined($filter->{'dual'}->{'content'})) {
            print STDERR "Dual filter not specified.\n";
            exit(1);
        }

        unless (defined($filter->{'dual'}->{'causality'})) {
            print STDERR "Dual filter causality not specified.\n";
            exit(1);
        }

        if (lc($filter->{'primary'}->{'causality'}) ne lc($filter->{'dual'}->{'causality'})) {
            print STDERR "Causality of primary and dual filters must be the same.\n";
            exit(1);
        }

        my $dual_coeffs = $filter->{'dual'}->{'content'};

        $dual_coeffs =~ s/^\s+//g;
        $dual_coeffs =~ s/\s+$//g;
        $dual_coeffs =~ s/\s+/ /g;

        $filter->{'dual'}->{'content'} = $dual_coeffs;
    }
}

sub even_alternate_sign {
    my @coeffs = @_;

    # Alternate sign starting from coeff[0]
    for (my $i = 0; $i < @coeffs; $i += 2) {
        if ($coeffs[$i] < 0) {
            $coeffs[$i] =~ s/^\-//;
        } else {
            $coeffs[$i] = '-'.$coeffs[$i];
        }
    }

    return @coeffs;
}

sub odd_alternate_sign {
    my @coeffs = @_;

    # Alternate sign starting from coeff[1]
    for (my $i = 1; $i < @coeffs; $i += 2) {
        if ($coeffs[$i] < 0) {
            $coeffs[$i] =~ s/^\-//;
        } else {
            $coeffs[$i] = '-'.$coeffs[$i];
        }
    }

    return @coeffs;
}

sub make_filterbank {
    my $filter = shift @_;

    my @lowpass_analysis_coeffs;
    my $lowpass_analysis_length;
    my $lowpass_analysis_causality;

    my @highpass_analysis_coeffs;
    my $highpass_analysis_length;
    my $highpass_analysis_causality;

    my @lowpass_synthesis_coeffs;
    my $lowpass_synthesis_length;
    my $lowpass_synthesis_causality;

    my @highpass_synthesis_coeffs;
    my $highpass_synthesis_length;
    my $highpass_synthesis_causality;

    if (lc($filter->{'type'}) eq 'biorthogonal') {
        # Make lowpass analysis filter
        @lowpass_analysis_coeffs = split(/ /, $filter->{'primary'}->{'content'});
        $lowpass_analysis_length = @lowpass_analysis_coeffs;
        $lowpass_analysis_causality = uc($filter->{'primary'}->{'causality'});

        # Make lowpass synthesis filter
        @lowpass_synthesis_coeffs = split(/ /, $filter->{'dual'}->{'content'});
        $lowpass_synthesis_length = @lowpass_synthesis_coeffs;
        $lowpass_synthesis_causality = uc($filter->{'dual'}->{'causality'});

        # Compute real filter length
        my $analysis_length =
            $lowpass_analysis_length * 2 -
           ($lowpass_analysis_causality eq 'SYMMETRIC_WHOLE');

        my $synthesis_length =
            $lowpass_synthesis_length * 2 -
           ($lowpass_synthesis_causality eq 'SYMMETRIC_WHOLE');

        # Make highpass analysis filter
        if (int($synthesis_length / 2) % 2) {
            @highpass_analysis_coeffs =
                even_alternate_sign(@lowpass_synthesis_coeffs);
        } else {
            @highpass_analysis_coeffs =
                odd_alternate_sign(@lowpass_synthesis_coeffs);
        }

        $highpass_analysis_length = @highpass_analysis_coeffs;
        $highpass_analysis_causality = $lowpass_analysis_causality;

        # Make highpass synthesis filter
        if (int($analysis_length / 2) % 2) {
            @highpass_synthesis_coeffs =
                odd_alternate_sign(@lowpass_analysis_coeffs);
        } else {
            @highpass_synthesis_coeffs =
                even_alternate_sign(@lowpass_analysis_coeffs);
        }

        $highpass_synthesis_length = @highpass_synthesis_coeffs;
        $highpass_synthesis_causality = $lowpass_analysis_causality; 
    } else {
        # Make lowpass analysis filter
        @lowpass_analysis_coeffs = split(/ /, $filter->{'primary'}->{'content'});
        $lowpass_analysis_length = @lowpass_analysis_coeffs;
        $lowpass_analysis_causality = 'ANTICAUSAL';

        # Make lowpass synthesis filter
        @lowpass_synthesis_coeffs = reverse(@lowpass_analysis_coeffs);
        $lowpass_synthesis_length = $lowpass_analysis_length;
        $lowpass_synthesis_causality = 'CAUSAL';

        # Make highpass analysis filter
        @highpass_analysis_coeffs = odd_alternate_sign(@lowpass_synthesis_coeffs);
        $highpass_analysis_length = $lowpass_analysis_length;
        $highpass_analysis_causality = 'ANTICAUSAL';

        # Make highpass synthesis filter
        @highpass_synthesis_coeffs = even_alternate_sign(@lowpass_analysis_coeffs);
        $highpass_synthesis_length = $lowpass_analysis_length;
        $highpass_synthesis_causality = 'CAUSAL';
    }

    # Print comment
    print "$filter->{'info'}\n";

    # Print lowpass analysis filter coeffs
    print "static coeff_t $filter->{'id'}_lowpass_analysis_coeffs\[\] = {\n";

    for (@lowpass_analysis_coeffs) {
        $_ < 0 ? print "   $_,\n" : print "    $_,\n";
    }

    print "};\n\n";

    # Print highpass analysis filter coeffs
    print "static coeff_t $filter->{'id'}_highpass_analysis_coeffs\[\] = {\n";

    for (@highpass_analysis_coeffs) {
        $_ < 0 ? print "   $_,\n" : print "    $_,\n";
    }

    print "};\n\n";

    # Print lowpass synthesis filter coeffs
    print "static coeff_t $filter->{'id'}_lowpass_synthesis_coeffs\[\] = {\n";

    for (@lowpass_synthesis_coeffs) {
        $_ < 0 ? print "   $_,\n" : print "    $_,\n";
    }

    print "};\n\n";

    # Print highpass synthesis filter coeffs
    print "static coeff_t $filter->{'id'}_highpass_synthesis_coeffs\[\] = {\n";

    for (@highpass_synthesis_coeffs) {
        $_ < 0 ? print "   $_,\n" : print "    $_,\n";
    }

    print "};\n\n";

    # Print lowpass analysis filter
    print "static filter_t $filter->{'id'}_lowpass_analysis = {\n";
    print "    $lowpass_analysis_length,\n";
    print "    $lowpass_analysis_causality,\n";
    print "    LOWPASS_ANALYSIS,\n";
    print "    $filter->{'id'}_lowpass_analysis_coeffs,\n";
    print "};\n\n";

    # Print highpass analysis filter
    print "static filter_t $filter->{'id'}_highpass_analysis = {\n";
    print "    $highpass_analysis_length,\n";
    print "    $highpass_analysis_causality,\n";
    print "    HIGHPASS_ANALYSIS,\n";
    print "    $filter->{'id'}_highpass_analysis_coeffs,\n";
    print "};\n\n";

    # Print lowpass synthesis filter
    print "static filter_t $filter->{'id'}_lowpass_synthesis = {\n";
    print "    $lowpass_synthesis_length,\n";
    print "    $lowpass_synthesis_causality,\n";
    print "    LOWPASS_SYNTHESIS,\n";
    print "    $filter->{'id'}_lowpass_synthesis_coeffs,\n";
    print "};\n\n";

    # Print highpass synthesis filter
    print "static filter_t $filter->{'id'}_highpass_synthesis = {\n";
    print "    $highpass_synthesis_length,\n";
    print "    $highpass_synthesis_causality,\n";
    print "    HIGHPASS_SYNTHESIS,\n";
    print "    $filter->{'id'}_highpass_synthesis_coeffs,\n";
    print "};\n\n";

    $filter->{'type'} = uc($filter->{'type'});

    # And finally, print filterbank
    print "static filterbank_t $filter->{'id'} = {\n";
    print "   \"$filter->{'id'}\",\n";
    print "   \"$filter->{'name'}\",\n";
    print "    $filter->{'type'},\n";
    print "    \&$filter->{'id'}_lowpass_analysis,\n";
    print "    \&$filter->{'id'}_highpass_analysis,\n";
    print "    \&$filter->{'id'}_lowpass_synthesis,\n";
    print "    \&$filter->{'id'}_highpass_synthesis,\n";
    print "};\n\n";
}

Main();
