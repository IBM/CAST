#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/temp/temp.pm
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#==============================================================================
# Check temperature of sensors on a node

use strict;
use warnings;

use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;
use ClustConf;
use YAML;

# SETUP: usage ----------------------------------------------------------------
sub usage {
    print "$0 [options]\n";
    print "    options\n";
    print "    -v|--verbose: set verbose on\n";
    print "    -h|--help : print this\n" ;
    exit 0
}

my $help=0;
my $verbose=0;
my $test=$0;
$test =~ s{.*/}{};      # removes path  
$test =~ s{\.[^.]+$}{}; # removes extension

GetOptions(
    'h|help'      => \$help,
    'v|verbose'   => \$verbose,
);
if ($help) { usage() }


# Get expected values ---------------------------------------------------------
my $Clustconf = new ClustConf;
$Clustconf->load();

my $node = `hostname -s`;
$node =~ s/\R//g; 
my $rc = 0;
my $errs = [];
my $c_high;
my $c_low;

my $nodeCfg = $Clustconf->findNodeCfg($node);
if (defined $nodeCfg) {
    $c_high = $nodeCfg->{temp}->{celsius_high};
    if (! defined $c_high) {
        push(@$errs, "$node: nodeCfg->{temp}->{celsius_high} failed");
        $rc = 1;
    }
    $c_low = $nodeCfg->{temp}->{celsius_low};
    if (! defined $c_low) {
        push(@$errs, "$node: nodeCfg->{temp}->{c_low} failed");
        $rc = 1;
    }
}

# Get temp information from node and compare values ---------------------------
my $tempdir = tempdir( CLEANUP => 1 );

if ($rc == 0) {
    my $cmd = "sensors | grep '[0-9]\.[0-9].C' 2>$tempdir/stderr";
    if ($verbose) {print "command: $cmd\n";}
    my $rval = `$cmd`;

    foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { 
        chomp $l; 
        push(@$errs,"WARN: $l");
    }

    if (length($rval) == 0) {
       push (@$errs, "(ERROR) Found no sensor temperatures\n");
    } else {
        foreach my $l (split(/\n/, $rval)) {
            # Format Ex:
            # Chip 8 Core 112:   +18.0°C  (lowest = +14.0°C, highest = +35.0°C)
            my ($sensor, $temp_values) = split /\s*:\s*/, $l;
            my $temp = substr $temp_values, 1, index($temp_values, "C") - 3;
            
            # Compare temp value to thresholds
            if ($temp < $c_low && $temp != 0) {
                push(@$errs, "$node: $sensor temperature under $c_low C. Temp: $temp C");
            }
            if ($temp > $c_high) {
                push(@$errs, "$node: $sensor temperature above $c_high C. Temp: $temp C");
            }
        }
    } 
}

# Print out errors ------------------------------------------------------------
if (scalar @$errs) {
    for my $e (@$errs) {
        print "(ERROR) $e\n";
    }
    print "$test test FAIL, rc=1\n";
    exit 1;
}
print "$test test PASS, rc=0\n";
exit 0;
