#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-cpu-count/chk-cpu-count.pm
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


use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use ClustConf;
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



my $errcnt = 0;
my $node = `hostname -s`;
$node =~ s/\R//g;
my $nodeCfg = $Clustconf->findNodeCfg($node);
if (!defined $nodeCfg ) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}
my $rc=0;
my $errs=[];  
my $exp_ncpus =$nodeCfg->{ncpus};
if (!defined $exp_ncpus) {
   push(@$errs, "$node: nodeCfg->{ncpus} failed");
   $rc=1;
}
my $tempdir = tempdir( CLEANUP => 1 );
if ($rc == 0) {
   my $cmd = "/usr/bin/lscpu | grep ^'CPU(s)'  2>$tempdir/stderr | sort";
   if ($verbose) {print "\ncommand: $cmd\n";}
   my $rval = `$cmd`;
   $rc=$?;

   if ($verbose) {print "output: $rval";}
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
   
   if (length($rval) == 0) { push(@$errs,"Found No command output: "); }

   # expecting a line like:
   # CPU(s):        80
   my ($ncpus) = $rval =~ /^\S+:\s+(\d+)$/;
   
   if ((!defined $ncpus) || ($ncpus != $exp_ncpus)) {
      push (@$errs, "$node: expecting $exp_ncpus cpus, got: $ncpus");
      $errcnt += 1;
   }
   else {
      print "$node: expecting: $exp_ncpus cpus, got:$ncpus\n";
   }
}
# print out the errors with an approrpiate error header...
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
} 
print "$test test PASS, rc=0\n";
exit 0;


