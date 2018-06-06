#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-memory/chk-memory.pm
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

# total memory, number of banks (dimms) and bank size
my $total=$nodeCfg->{memory}->{total};
my $banks=$nodeCfg->{memory}->{banks};
my $size=$nodeCfg->{memory}->{bank_size};

if (!defined $total ) {
   push(@$errs, "$node: $nodeCfg->{memory}->{total} failed");
   $rc=1;
}

if (!defined $banks ) {
   push(@$errs, "$node: $nodeCfg->{memory}->{banks} failed");
   $rc=1;
}

if (!defined $size ) {
   push(@$errs, "$node: $nodeCfg->{memory}->{size} failed");
   $rc=1;
}

my $STR1="System memory";
my $STR2="DIMM";

### bank size is supported yet

my $tempdir = tempdir( CLEANUP => 1 );

# we should use json, but using -short for now
# json file format error
my $cmd = "sudo /usr/sbin/lshw -class memory -short -quiet | egrep '$STR1|$STR2' 2>$tempdir/stderr";
if ($verbose) {print "\ncommand: $cmd\n";}
my $rval = `$cmd`;
$rc=$?;
if ($verbose) {print "output: $rval";}
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
if (length($rval) == 0) { push(@$errs,"Found No command output: "); }

my $count=0;
## /0/2/c                    memory     32GiB RDIMM DDR4 2666 MHz (0.4ns)
## /0/2/c                    memory     RDIMM DDR4 2666 MHz (0.4ns)
my $newfirm=1;
foreach my $line (split(/\n/,$rval)) {
   #print $line,"\n";
   if ($line =~ /$STR2/ ) {
     ++$count;
     my ($rsize) = $line =~  /^\S+\s+memory\s+(\d+)GiB RDIMM.+/;
     if ( defined $rsize ) {
        if ( $rsize != $size) {
           push(@$errs,"Bank size expected: $size, got: $rsize."); 
        }
     } 
     else {
        $newfirm=0;
        if ($verbose) {print "Firmware version does not support bank size.\n";}
     }
   }
   else {
     my ($value) = $line =~  /^\S+\s+memory\s+(\d+)GiB System memory/;
     if ( defined $value ) {
        # total line
        if ( $value != $total) {
           push(@$errs,"Total memory size expected: $total, got: $value."); 
        }   
        else { print "Total memory size expected: $total\n";}
     }
     else { push(@$errs,"Unexpected line found.");}
   }
}
if ( $count != $banks ) {
   push(@$errs,"Number of banks expected: $banks, got: $count."); 
}
else { 
   print "Total number of banks expected: $banks\n";
   if ( $newfirm ) { print "Bank size: $size\n";}
}
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


