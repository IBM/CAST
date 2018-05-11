#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/ipoib/ipoib.pm
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
# Show the state, MTU, and mode of the IPoIB devices on the node

use strict;
use warnings;

use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;


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


# Print IPoIB info ------------------------------------------------------------
my $node = `hostname -s`;
$node =~ s/\R//g;
my $tempdir = tempdir( CLEANUP => 1 );
my $rc=$?;
my $errs = [];

my $ib_cmd = "ls /sys/class/net/ | grep 'ib.' 2>$tempdir/stderr";
if ($verbose) { print "List ibs command: $ib_cmd\n"; }
my $ib_list = `$ib_cmd`;

if (length($ib_list) == 0) {
    push (@$errs, "(ERROR) Found no IPoIB devices\n");
}

foreach my $ib (split /\n/, $ib_list) {
    my $state_cmd = "cat /sys/class/net/$ib/operstate 2>$tempdir/stderr";
    my $mtu_cmd = "cat /sys/class/net/$ib/mtu 2>$tempdir/stderr";
    my $mode_cmd = "cat /sys/class/net/$ib/mode 2>$tempdir/stderr";
    if ($verbose) {print "Info commands:\n$state_cmd\n$mtu_cmd\n$mode_cmd\n\n";}
    my $state = `$state_cmd`;
    my $mtu = `$mtu_cmd`;
    my $mode = `$mode_cmd`;

    if (! defined $state || ! defined $mtu || ! defined $mode) {
      push (@$errs, "(ERROR) Missing files in /sys/class/net/$ib/\n");
    } else {
      print "$ib -------------------------------------\n";
      print "State: " . uc $state;
      print "MTU: $mtu";
      print "Mode: " . uc $mode . "\n";
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
