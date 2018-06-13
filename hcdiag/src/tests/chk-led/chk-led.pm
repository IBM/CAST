#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/chk-led/chk-led.pm
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
# Check noderange for fault leds

use strict;
use warnings;

use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;

# SETUP: usage ----------------------------------------------------------------
sub usage {
   print "$0 [options] noderange\n";
   print "    options\n";
   print "    -v|--verbose: set verbose on\n";
   print "    -h|--help: print this\n" ;
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


# Get led information ---------------------------------------------------------
my $errs = [];
my $tempdir = tempdir( CLEANUP => 1 );

my $nodeRange = shift @ARGV || do {
   print "Mandatory argument is missing\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
};
if ($verbose) { print "nodes = ".Dumper($nodeRange)."\n"; }

my $cmd = "rvitals $nodeRange leds | grep 'Fault:\s*On' 2>$tempdir/stderr";
if ($verbose) { print "$cmd\n"; }
my $rval = `$cmd`;
my $rc=$?;

foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   print "(WARNING) $l\n";
}

if (length($rval) != 0) {
   foreach my $l (split(/\n/, $rval)) {
      chomp $l;
      if ($verbose) { print "$l\n"; }
      my ($node,$fault) = (split /\s*[:\.]\s*/, $l)[0,1];
      $fault =~ s/\s*Fault//g;
      if ((defined $node) && (defined $fault)) {
         push(@$errs, "$node: Fault led at $fault\n");
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
