#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/ibcredit/ibcredit.pm
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
# Check for credit loops

use strict;
use warnings;

use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;
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


# Use ibdiagnet to check for credit loops -------------------------------------
my $tempdir = tempdir( CLEANUP => 1 );
my $rc=$?;
my $errs = [];

my $cmd = "sudo /usr/bin/ibdiagnet -r --fat_tree --skip all -o $tempdir/ibdiagnet 2>$tempdir/stderr";
if ($verbose) {print "command: $cmd\n";}
my $rval = `$cmd`;

if (`wc -l $tempdir/stderr` eq 0) {
    # Parse the log file for the credit loop report
    my $check = "cat $tempdir/ibdiagnet/ibdiagnet2.log | grep 'no credit loops found' | wc -l";
    if ($verbose) {print "command: $check\n";}
    my $loop = `$check`;

    if ($loop eq 0) {
        my $report = `cat $tempdir/ibdiagnet/ibdiagnet2.log`;
        push(@$errs, "Credit loop(s) found. See report:\n\n$report");
    }
} else {
    foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { 
        chomp $l; 
        push(@$errs,"WARN: $l");
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
