#!/usr/bin/perl

#================================================================================
#   
#    hcdiag/src/tests/chk-nvlink-speed/chk-nvlink-speed.pm
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
   print "$0 [options] \n";
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


my $node = `hostname -s`;
$node =~ s/\R//g;

my $Clustconf = new ClustConf;
$Clustconf->load();
my $nodeCfg = $Clustconf->findNodeCfg($node);
if ( !defined $nodeCfg ) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}
my $gpus=$nodeCfg->{gpu}->{pciids};
my $speed=$nodeCfg->{gpu}->{link_speed};
my $nlinks=$nodeCfg->{gpu}->{nlinks};
my $ngpus=scalar(@$gpus);
$nlinks=$ngpus*$nlinks;

my $tempdir = tempdir( CLEANUP => 1 );
$speed="$speed GB/s";

my $errMsgs=[];
my $cmd = "nvidia-smi nvlink -s";
print $cmd . "\n";

my $rval = `$cmd`;
my $rc=$?;
if ($rc !=0 ) {
   print "(WARNING) failure in $cmd\n";
   system("cat $tempdir/stderr");
}

if($verbose) { print "$rval\n"; }
my @matches = $rval =~ /($speed)/g;
my $count = @matches;
  
if ( $count != $nlinks ) {
   print "ERROR: Expected $nlinks links at $speed, got $count\n";
   print "$test test FAIL, rc=1\n";
   exit 1
}

print "$test test PASS, rc=0\n";
exit 0;

