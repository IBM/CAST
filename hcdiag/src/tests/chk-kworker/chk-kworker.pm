#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-kworker/chk-kworker.pm
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

use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;

sub usage {
   print "$0 [options]\n";
   print "    options\n";
   print "    -v|--verbose: set verbose on\n";
   print "    -h|--help : print this\n" ;
   exit 0
}

my $help=0;
my $verbose=0;

GetOptions(
    'h|help'      => \$help,
    'v|verbose'   => \$verbose,
);

if ($help) { usage() }

my $node = `hostname -s`;
$node =~ s/\R//g;

my $tempdir = tempdir( CLEANUP => 1 );
my $cmd = "ps r --no-header  -o %cpu,stat,cmd \$(pgrep kworker) 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;

print"==> $rval\n";

my $rec={};
my $errs = [];

foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   print "(WARN) $l\n";
}
foreach my $l (split(/\n/,$rval)) {
   # parse off:
   # c460c004: 40.3 R    [kworker/u320:3]
   chomp $l;
   if ($verbose) {print $l . "\n";}

   my ($cpu,$state,$cmd) = $l =~ /^\s*([\d\.]+)\s*(\S)\s*(.+?)$/;
   print "==> $cpu, $state, $cmd" ;
   if ((defined $node) && (defined $cpu) && (defined $state) && (defined $cmd)) {
      if ($cpu >= 20.0) {
         push (@$errs, $l . ": unexpectly busy kworker: " );
         #print "node=$node,cpu=$cpu,state=$state,cmd=$cmd\n";
      }
   }
}

# print out the errors with an approrpiate error header...
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "FAILED\n";
   exit 1;
} 


print "SUCCESS\n";
exit 0;


