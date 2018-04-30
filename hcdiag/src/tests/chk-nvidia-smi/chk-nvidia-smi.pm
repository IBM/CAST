#!/usr/bin/perl

#================================================================================
#   
#    hcdiag/src/tests/chk-nvidia-smi/chk-nvidia-smi.pm
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

my $tempdir = tempdir( CLEANUP => 1 );
my $node = `hostname -s`;

my $cmd = "ps  -o comm,user,state,lstart,command --no-headers -C nvidia-smi || true 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;
if ($rc !=0 ) {
   print "(WARNING) failure in $cmd\n";
   system("cat $tempdir/stderr");
}

#
# parse any nvidia-smi present...
# simpliest possible test, but we could have a race condition...
#
# todo: parse off the date and anything less than a minute old gets flagged.
# 
# 

# ps command states
# D   uninterruptible sleep (usually IO)
# R   runnable (on run queue)           
# S   sleeping                          
# T   traced or stopped                 
# Z   a defunct ("zombie") process      
# now we need to parse stuff that looks like this.
my $errtext=[];  
foreach my $l ( split('\n',$rval) ) {
   if ($verbose) { print "$l\n";}

   # c460login01: nvidia-smi      sekiyama S Sun Feb 26 04:54:27 2017 nvidia-smi -l
   my ($node, $comm, $user,$state,$lstart,$command) = 
     $l =~ m/^(\S+):\s(\S+)\s+(\S+)\s+(\S+)\s+(\S+\s+\S+\s+\S+\s+\d+:\d+:\d+\s+\d+)\s+(.*)$/;
   if ($verbose) {
     print Dumper($node, $comm, $user,$state,$lstart,$command) . "\n";
   }

   if (((! defined $state) || ($state !~ m/(S|R)/) ) || 
      ((! defined $command ) || ($command !~ m/.*\s(-l(\s|$)|-l\S+|-lms|--loop=).*/)) ){
      if ($verbose) { print "push(errtext " . $l ."\n";}
      push(@$errtext,"$l");
   }
}
# if it has a nvidia-smi ' -l' init then it means we should ignore it.

if (scalar(@$errtext) > 0) {
   print "(ERROR) Found possible stuck nvidia-smi modules\n";
   for my $e (@$errtext) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
}

print "$test test PASS, rc=0\n";
exit 0;


