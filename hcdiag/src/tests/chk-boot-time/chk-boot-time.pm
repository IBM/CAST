#!/usr/bin/perl                          
#================================================================================
#   
#    hcdiag/src/tests/chk-boot-time/chk-boot-time.pm
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
use Date::Parse;

#
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
my $cmd = "nodestat $nodeRange 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
if ($verbose) {print "output: $rval";}
my $rc=$?;
if ($rc !=0 ) {
   push(@$errMsgs,"(ERROR) in cmd rc=$rc\n");
}

# all stderr are errors for this test.
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   push(@$errMsgs,"$l");
}



my $now=time;
my $nodes = [];
foreach my $l (split(/\n/,$rval)) {
   my ($node,$status) = $l =~ /^(\S+): (\S+).*/;
   if ($status ne 'sshd') {
      push(@$nodes,$node);
   }  
   else {
      print "skipping $node, status: $status.\n" if ($verbose);
   }
}

if ( scalar(@$nodes) > 0) { 
   my $newnoderange=join( ',', @$nodes );
   $cmd = "lsdef $newnoderange -c -i status,statustime 2>$tempdir/stderr";
   print $cmd . "\n";
   $rval = `$cmd`;
   my $rc=$?;
   if ($rc !=0 ) {
      push(@$errMsgs,"(ERROR) in cmd rc=$rc\n");
   }
   if ($verbose) {print "output: $rval";}
   
  
   # all stderr are errors for this test.
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
      chomp $l;
      push(@$errMsgs,"$l");
   }

   #  c650f99p04: status=booted
   #  c650f99p04: statustime=04-16-2018 11:54:59
   #  c650f99p06: status=powering-on
   #  c650f99p06: statustime=03-12-2018 08:02:53
   # let's build <node, status, time>
   struct ( NodeStatusItem => [  
      name => '$',
      status => '$',
      statustime => '$'
   ]);                                          

   my $nodeStatus = {};
   foreach my $l (split(/\n/,$rval)) {
      my ($name,$attr,$value) = $l =~ /(\S+): (\S+)=(.*)$/;
      if (! ((defined $name) && (defined $attr) && (defined $value))) {
          print "skipping unrecognized value: $l\n";
          next;
      }
      if (!exists $nodeStatus->{$name}) {
          $nodeStatus->{$name} = NodeStatusItem->new("name" => $name,
                                                     "status" => "",
                                                     "statustime" => "");
      }
      if ($attr eq "status") {
          $nodeStatus->{$name}->status($value);
      } else {
          $nodeStatus->{$name}->statustime($value);
      }
   }
   # now that we have all nicely
   for my $n (sort keys %$nodeStatus) {
      my $it = $nodeStatus->{$n};
      if ($verbose) { print "it=".Dumper($it)."\n"; }
   
      if (($it->status ne "booted") && ($it->status ne "failed")) {  
          if (length($it->statustime) == 0) { next; }
       
          my $statustime=str2time($it->statustime);
          if (($now-$statustime) > (60*12)) {
             push(@$errMsgs, "(ERROR) $n: failed to boot.  status=".$it->status.",statustime=".$it->statustime);
          }
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


