#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-nvme-mount/chk-nvme-mount.pm
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
#=============================================================================
# simple test to check if the switch configuraton matches
# that installed on the switches.

use strict;
use warnings;

use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;

sub usage {
   print "$0 [options] [nvme_mount_point]\n";
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


my $mountPoint = shift @ARGV || "/nvme";
print "Checking mount point: $mountPoint\n";


my $node = `hostname -s`;
$node =~ s/\R//g;

my $tempdir = tempdir( CLEANUP => 1 );
my $cmd = "sudo df -h $mountPoint 2>$tempdir/stderr | sed 1d  | sort";
if ($verbose) {print "command: $cmd\n";}

my $rval = `$cmd`;
my $rc=$?;
if ($rc !=0 ) {
   print "(ERROR) failure in $cmd\n";
   print "retval = $rval";
   print "$test test FAIL, rc=1\n";
   exit 1;
}

my $errs = [];
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }

if (length($rval) == 0) {
   push (@$errs, "(ERROR) Found No NVMe devices mounted \n");
}

# we are looking for a line like this
# /dev/nvme0n1    1.5T   77M  1.4T   1% /nvme
foreach my $l (split(/\n/,$rval)) {
   my ($dev) = $l =~ /^(\S+) .*/;
   if ( $verbose ) { print "$l -- dev=$dev\n"; }
   if ($dev ne "/dev/nvme0n1") {
      push (@$errs, "(ERROR) nvme device not mounted or not found: $l\n");
   }
}

# print out the errors with an appropriate error header...
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
} 
print "$test test PASS, rc=0\n";
exit 0;


