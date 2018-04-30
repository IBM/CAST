#!/usr/bin/perl                          
#================================================================================
#   
#    hcdiag/src/tests/rpower/rpower.pm
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

sub usage {
   print "$0 [options] noderange\n";
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

my $nodeRange = shift @ARGV || do {
  print "Mandatory argument is missing\n";
  print "$test test FAIL, rc=1\n";
  exit 1;
};

#if ($verbose) { print "nodes=".Dumper($nodeRange)."\n"; }

my $errMsgs=[];
my $tempdir = tempdir( CLEANUP => 1 );
my $cmd = "rpower $nodeRange status 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;
if ($rc !=0 ) {
   push(@$errMsgs,"(ERROR) in cmd rc=$rc\n");
}

# all stderr are errors for this test.
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   push(@$errMsgs,"$l");
}

foreach my $l (split(/\n/,$rval)) {
   #print "$l\n"
   my ($node,$status) = $l =~ /^(\S+): (\S+).*/;
   if ($status ne 'on') {
      push(@$errMsgs,"$node: $status");
   }  
   else {
      if ($verbose) {
         print "$node, status: $status.\n"
      }
   }
}


if (scalar(@$errMsgs) > 0) {
   for my $e (@$errMsgs) {
      print("(ERROR) $e\n");
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
}


print "$test test PASS, rc=0\n";
exit 0;


