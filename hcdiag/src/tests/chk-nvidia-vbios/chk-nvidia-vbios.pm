#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-nvidia-vbios/chk-nvidia-vbios.pm
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
use File::Temp qw/ tempdir /;
use YAML;

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


#my $node = "c650f99p06";
my $node = `hostname -s`;
$node =~ s/\R//g;
my $nodeCfg = $Clustconf->findNodeCfg($node);
if (!defined $nodeCfg) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}


my $tempdir = tempdir( CLEANUP => 1 );
my $cmd = "nvidia-smi --query-gpu=gpu_name,gpu_bus_id,vbios_version --format=noheader,csv 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;


#
# this is what we are paring csv, and trim the leading space...
# Tesla P100-SXM2-16GB, 0002:01:00.0, 86.00.26.00.02
# Tesla P100-SXM2-16GB, 0003:01:00.0, 86.00.26.00.02
# Tesla P100-SXM2-16GB, 0006:01:00.0, 86.00.26.00.02
# Tesla P100-SXM2-16GB, 0007:01:00.0, 86.00.26.00.02


# gather the firmware levels.
my $rec={};
my $errs = [];

foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   print "(WARN) $l";
}

foreach my $l (split(/\n/,$rval)) {
   chomp $l;
   if ($verbose) {print $l . "\n";}

   # 384 driver reports bus id with 8 leading zeros not 4, as in previous drivers...
   # Tesla P100-SXM2-16GB, 00000002:01:00.0, 86.00.41.00.01
   my ($dev,$busid,$vbios) = $l =~ /^(.+?),\s*(.+?),\s*(.+?)$/;
   if ( (defined $dev) && (defined $busid) && (defined $vbios)) {
      # convert to the common xxxx:xx:xx:xx format...
      $busid =~ s/0000(\d\d\d\d:)/$1/;
      
      $rec->{$busid}->{vbios} = $vbios;
      $rec->{$busid}->{dev} = $dev;
   }
}
if ($verbose) {print "vbios=".Dumper($rec)."\n"};



# check the firmware levels.
my $err=0;
my $i = $rec;
if ($verbose) {print "vbios=".Dumper($i)."\n"};
# we are expecting specific vbios levels on specific devices.

my $bidlist =   $nodeCfg->{gpu}->{pciids};
if (! defined $bidlist) {
   push(@$errs, "$node: nodeCfg->{gpu}->{pciids} failed");
   next;
}
my $exp_vbios=$nodeCfg->{gpu}->{vbios};
if (! defined $exp_vbios) {
   push(@$errs, "$node: nodeCfg->{gpu}->{vbios} failed");
   next;
}
my $exp_dev = $nodeCfg->{gpu}->{device};
if (! defined $exp_dev) {
   push(@$errs, "$node: nodeCfg->{gpu}->{device} failed");
   next;
}
for my $bid (@$bidlist) {
   my $busid=$bid;
   if (! defined $rec->{$busid}) {
      push (@$errs, "$node: missing device $busid");
      next
   }
   if (($rec->{$busid}->{dev} ne $exp_dev) ||
       ($rec->{$busid}->{vbios} ne $exp_vbios)) {
      push (@$errs, "$node: invalid vbios: " . 
                    $rec->{$busid}->{dev}.','.
                    $rec->{$busid}->{vbios} . '; ' .
                    "expected: $exp_dev, $exp_vbios");
   }
   else {
      print "PCI bus: $busid, $exp_dev, $exp_vbios match\n";
   }
}

# print out the errors with an approrpia  te error header...
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
} 


print "$test test PASS, rc=0\n";
exit 0;


