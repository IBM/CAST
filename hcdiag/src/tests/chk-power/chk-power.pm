#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/chk-power/chk-power.pm
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
# Check power capping values on node, compare to current power usage

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

# Get device information from node and compare values -------------------------
my $node = `hostname -s`;
$node =~ s/\R//g; 
my $errs = [];
my $tempdir = tempdir( CLEANUP => 1 );

# Node power cap value
my $pwrcap_cmd = "cat /sys/firmware/opal/powercap/system-powercap/powercap-current 2>$tempdir/stderr";
if ($verbose) {print "command: $pwrcap_cmd\n";}
my $pwrcap = `$pwrcap_cmd`;
chomp $pwrcap;
print "$node Power Cap: $pwrcap W\n\n";

# Node power shift ratios
my $pwrshift_cmd = "ls /sys/firmware/opal/psr/ 2>$tempdir/stderr";
my $pwrshift = `$pwrshift_cmd`;
my @ids = split("\n",$pwrshift);
foreach my $id (@ids) {
    my $ratio_cmd = "cat /sys/firmware/opal/psr/$id 2>$tempdir/stderr";
    if ($verbose) {print "command: $ratio_cmd\n";}
    my $ratio = `$ratio_cmd`;
    chomp $ratio;
    print "$id Power Shift Ratio: $ratio\n";
}

# Current system power
my $pwr_cmd = "sensors 2>$tempdir/stderr | awk '/System/ {print \$2,\$3}'";
if ($verbose) {print "command: $pwr_cmd\n";}
my $pwr = `$pwr_cmd`;
print "\nSystem power: $pwr\n";

# Chip power
my $chip_cmd = "sensors 2>$tempdir/stderr | grep '^Chip.*W'";
if ($verbose) {print "command: $chip_cmd\n";}
my $chip_pwr = `$chip_cmd`;
print "Chip Power Values---------------------------------------------------------\n";
print "$chip_pwr\n";

# GPU power values
my $gpu_cmd = "nvidia-smi -q -d POWER | grep -A 4 'GPU'";
if ($verbose) {print "command: $gpu_cmd\n";}
my $gpu_power =`$gpu_cmd`;
if($gpu_power !~ /Power Draw/) {
    push(@$errs, "$node: Error reading GPU power values.\nCheck command: $gpu_cmd");
}
print "Detailed GPU Power Info---------------------------------------------------\n";
print $gpu_power;
 
# Print out errors ------------------------------------------------------------
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { 
    chomp $l; 
    push(@$errs,"WARN: $l");
}

if (scalar @$errs) {
    for my $e (@$errs) {
        print "(ERROR) $e\n";
    }
    print "$test test FAIL, rc=1\n";
    exit 1;
}
print "$test test PASS, rc=0\n";
exit 0;
