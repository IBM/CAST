#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-zombies/chk-zombies.pm
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

use strict;
use warnings;

use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";

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


#my $node = "c650f99p06";
my $node = `hostname -s`;
$node =~ s/\R//g;

my $tempdir = tempdir( CLEANUP => 1 );
my $cmd = "ps -e -o state,ppid,pid,comm,lstart,user,command --no-headers | egrep \'^Z\\s+1\\s+\' || true 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;

foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   print "(WARN) $l\n";
}


if (length($rval) > 0) {
   print "(ERROR) Found possible stuck zombie tasks\n";
   print "$rval\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}

print "$test test PASS, rc=0\n";
exit 0;


