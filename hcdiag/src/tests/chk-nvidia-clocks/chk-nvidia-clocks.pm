#!/usr/bin/perl
# simple test to check nvidia clocks

#================================================================================
#   
#    hcdiag/src/tests/chk-nvidia-clocks/chk-nvidia-clocks.pm
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
use ClustConf;

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

my $Clustconf = new ClustConf;
$Clustconf->load();

my $node = `hostname -s`;
$node =~ s/\R//g;

my $nodeCfg = $Clustconf->findNodeCfg($node);
if (!defined $nodeCfg) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test FAIL, rc=1\n";
   exit 1;
}
my $rc=0;
my $errs=[];  
my $app_clock_freq =   $nodeCfg->{gpu}->{clocks_applications_gr};
my $mem_clock_freq =   $nodeCfg->{gpu}->{clocks_applications_mem};
my $persist        =   $nodeCfg->{gpu}->{persistence_mode};
if (!defined $app_clock_freq) {
   push(@$errs, "$node: nodeCfg->{gpu}->{clocks_applications_gr} failed");
   $rc=1;
}
if (!defined $mem_clock_freq) {
   push(@$errs, "$node: nodeCfg->{gpu}->{clocks_applications_mem} failed");
   $rc=1;
}
if (!defined $persist) {
   push(@$errs, "$node: nodeCfg->{gpu}->{persistence_mode} failed");
   $rc=1;
}
# =====================
# clocks.applications.gr,clocks.applications.mem,persistence_mode
# nvidia-smi --query-gpu=clocks.applications.gr,clocks.applications.mem,persistence_mode --format=csv --noheader
# =====================
my $tempdir = tempdir( CLEANUP => 1 );
if ($rc == 0) {

   print "Expected GPU application clock frequency: $app_clock_freq MHz\n";
   print "Expected GPU memory clock frequency     : $mem_clock_freq MHz\n";
   print "Expected GPU persistence mode           : $persist\n";

   my $cmd = "/usr/bin/nvidia-smi --query-gpu=clocks.applications.gr,clocks.applications.mem,persistence_mode --format=csv | sed  1d 2>$tempdir/stderr";
   if ($verbose) {print "command: $cmd\n";}
   my $rval = `$cmd`;
   $rc=$?;
   my $len=length($rval);
   if ( ($rc !=0 ) || ( $len == 1 )) {
      print "(WARNING) failure in $cmd\n";
      system("cat $tempdir/stderr");
      print "$test test FAIL, rc=1\n";
      exit 1;
   }

   foreach my $l ( split('\n',$rval) ) {
      if ($verbose) { print "$l\n";}
      my ($clock_gr, $clock_mem,$pm) = $l =~ m/^(\S+)\s+\S+,\s(\S+)\s+\S+,\s(\S+)$/;
      if ( ! defined $clock_gr || ! defined $clock_mem || ! defined $pm) {
         push(@$errs, "cannot parse: $l");
         next;
      }
   
      if ($app_clock_freq ne $clock_gr) {
         print "$l\n";
         push (@$errs, "$node: incorrect clocks.applications.gr: $clock_gr, expected $app_clock_freq");
      }
      if ($mem_clock_freq ne $clock_mem) {
         print "$l\n";
         push (@$errs, "$node: incorrect clocks.applications.mem: $clock_mem, expected $mem_clock_freq");
      }
      if ($persist ne $pm) {
         print "$l\n";
         push (@$errs, "$node: incorrect persistance.mode : $pm, expected $persist");
      }
   }
}
# if it has a nvidia-smi ' -l' init then it means we should ignore it.

if (scalar(@$errs) > 0) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
}
print "$test test PASS, rc=0\n";


exit 0;


