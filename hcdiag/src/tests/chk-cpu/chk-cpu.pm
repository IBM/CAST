#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-cpu/chk-cpu.pm
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
# Check cpu attributes: 
#   count 
#   frequency 
#   memory
# also prints output of the following commands:
#  /usr/bin/lscpu
#  /usr/sbin/ppc64_cpu --smt
#  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | sort -u
#  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq | sort -u
#  cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq | sort -u
#  /usr/sbin/ppc64_cpu --subcores-per-core
#  /usr/sbin/ppc64_cpu --dscr
#  /usr/bin/numactl --hardware

use strict;
use warnings;

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
#my $node = "c650f99p06";
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
my $exp_max =$nodeCfg->{clock}->{max};
my $exp_min =$nodeCfg->{clock}->{min};
if (!defined $exp_ncpus) {
   push(@$errs, "$node: nodeCfg->{ncpus} failed");
   $rc=1;
}
if (!defined $exp_max) {
   push(@$errs, "$node: nodeCfg->{clock}->{max} failed");
   $rc=1;
}
if (!defined $exp_min) {
   push(@$errs, "$node: nodeCfg->{clock}->{min} failed");
   $rc=1;
}

# ================================
# Info only - no check
# ================================

print "\n'/usr/bin/lscpu' output:\n"; print `/usr/bin/lscpu`;
print "\n"; print `/usr/sbin/ppc64_cpu --smt`;
print "\nscaling_governor:\n"; print `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | sort -u`;
print "\nscaling_max_freq:\n"; print `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq | sort -u`;
print "\nscaling_min_freq:\n"; print `cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq | sort -u`;
print "\n"; print `/usr/sbin/ppc64_cpu --subcores-per-core`;
print "\n"; print `sudo /usr/sbin/ppc64_cpu --dscr`;
print "\n`/usr/bin/numactl --hardware' output: \n"; print `/usr/bin/numactl --hardware`;

print "\n";

# =====================
# check number of cpus 
# =====================
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
   print "-- node=$node, ncpus=$ncpus, expected_ncpus=$exp_ncpus\n";
   
   if ((!defined $ncpus) || ($ncpus != $exp_ncpus)) {
      push (@$errs, "$node: expecting $exp_ncpus cpus, got: $ncpus");
      $errcnt += 1;
   }
   
   # =====================
   # check cores online
   # =====================
   $cmd = "/usr/sbin/ppc64_cpu --cores-present";
   if ($verbose) {print "\ncommand: $cmd\n";}
   $rval = `$cmd`;
   $rc=$?;
   
   if ($verbose) {print "output: $rval";}
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
   
   # parse all values here...
   if (length($rval) == 0) { push(@$errs,"Found No command output: "); }
   
   # looking for a line like:
   # Number of cores online = 10
   my ($cores_present) = $rval =~ /^.+=\s+(\d+)$/;

   $cmd = "/usr/sbin/ppc64_cpu --cores-on";
   if ($verbose) {print "\ncommand: $cmd\n";}
   $rval = `$cmd`;
   $rc=$?;
   
   if ($verbose) {print "output: $rval";}
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
   
   if (length($rval) == 0) { push(@$errs,"Found No command output: "); }
   my ($cores_online) = $rval =~ /^.+=\s+(\d+)$/;
   
   if ((!defined $cores_present) || (!defined $cores_online) || ($cores_present != $cores_online)) {
      push (@$errs, "$node: not all the cores are online, expecting: $cores_present, got: $cores_online");
      $errcnt += 1;
   }
   
   # =====================
   # check cpu frequency
   # =====================
   $cmd = "sudo /usr/sbin/ppc64_cpu --frequency -t 1";
   if ($verbose) { print "\ncommand: $cmd\n";}
   $rval = `$cmd`;
   $rc=$?;
   
   if ($verbose) {print "output: $rval";}
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
   
   # parse all values here...
   if (length($rval) == 0) { push(@$errs,"Found No command output: "); } 
   
   foreach my $l (split(/\n/,$rval)) {
      # looking for a lines like:
      # min:    3.507 GHz (cpu 72)
      # max:    3.507 GHz (cpu 2)
      # avg:    3.507 GHz
      my ($min) = $l =~ /^min:\s+(\S+)/;
      my ($max) = $l =~ /^max:\s+(\S+)/;
      my ($avg) = $l =~ /^avg:\s+(\S+)/;
      if (defined $min) {
         print "-- node=$node, min clock frequency read=$min, allowed=$exp_min\n";
         if ($min < $exp_min) {
            push (@$errs, "$node: min clock frequency allowed: $exp_min, got: $min");
            $errcnt += 1;
         }
      }
   
      if ( defined $max) {
        print "-- node=$node, max clock frequency read=$max, allowed=$exp_max\n";
        if ($max > $exp_max) {
           push (@$errs, "$node: max clock frequency allowed: $exp_max, got: $max");
           $errcnt += 1;
        }
      }
   }
}   
print "\n";
   
# ================================
# Summary
# ================================

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


