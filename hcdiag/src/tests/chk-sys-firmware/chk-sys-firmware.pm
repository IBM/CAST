#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-sys-firmare/chk-sys-firmare.pm
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

my $Clustconf = new ClustConf;
$Clustconf->load();


my $tempdir = tempdir( CLEANUP => 1 );

my $nodeRange = shift @ARGV || do {
  print "Mandatory argument is missing\n";
  print "$test test FAIL, rc=1\n";
  exit 1;
};

if ($verbose) { print "nodes=".Dumper($nodeRange)."\n"; }

my $cmd = "rinv $nodeRange firm 2>$tempdir/stderr";
if ($verbose) {print "$cmd\n";}


# check the firmware,
# return a list of nodes in error
# and the error text to print.
sub checkFirmware {
   my ($nodes) = @_;
   my $errNodes= {};
   my $errs = [];
   
   if (length($nodeRange)==0) {
      print "(INFO) no nodes found to test\n";
      print "$test test PASS, rc=0\n";
      exit 0;
   }
   # need to capture stdout separatly.
   my $tempdir = tempdir( CLEANUP => 1 );
   
   # pass 1...
   my $cmd = "rinv $nodeRange firm 2>$tempdir/stderr";
   #if ($verbose) {print $cmd . "\n";}
   print $cmd . "\n";
   my $rval = `$cmd`;
   my $rc=$?;
   
   # gather the firmware levels.
   my $firmware={};
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
      chomp $l;
      #push (@$errs, $l);
      print "(WARNING) $l\n";
   }
   foreach my $l (split(/\n/,$rval)) {
      chomp $l;
      if ($verbose) {print $l . "\n";}
      my ($node,$value) = $l =~ /^(\S+): (.*)$/;
      if ((defined $node) && (defined $value)) {
         if (! defined $firmware->{$node}) {
            $firmware->{$node}->{node} = $node; 
            $firmware->{$node}->{vals} = [];
         }
         push(@{$firmware->{$node}->{vals}}, $value);
      }
   }
   if ($verbose) {print "firmware=".Dumper($firmware)."\n"};
   
   # we are looking to match all the strings in our array to the rinv.
   #   errors on closest missmatch
   #   errors on no match at all
   #
   # check the firmware levels.
   for my $node (sort keys $firmware) {
      my $fnode = $firmware->{$node};
      my $err=0;
      my $nodeCfg = $Clustconf->findNodeCfg($node);
   
   
      if (!defined $nodeCfg ) {
         push(@$errs, "$node: Clustconf->findNodeCfg($node) failed");
         $errNodes->{$node} = $node;
         next;
      }
      if ($verbose) {print "firmware=".Dumper($nodeCfg->{firmware})."\n"};
   
      # we must match ALL the strings in the version list.
      my $fwName = $nodeCfg->{firmware}->{name}; # release name
      if (! defined $fwName ) {
         push(@$errs, "cannot find firmware->name in $node yaml file");
         $errNodes->{$node} = $node;
         next;
      }
         
            
      my $expVers = $nodeCfg->{firmware}->{versions};
      if (! defined $expVers ) {
         push(@$errs, "cannot find firmware->versions in $node yaml file");
         $errNodes->{$node} = $node;
         next;
      }
      my $actVers = $fnode->{vals};
      # indexes of the values we have matched.
      my $expMatched = [];  @$expMatched = (0) x (scalar @$expVers);
      my $actMatched = [];  @$actMatched = (0) x (scalar @$actVers);
   
   
      # matched or close matched will mark these as matched and accounted for.
      # two passes, find all the exact matches, then go after the rest to find the
      # closest to print the errors.
      for (my $expIdx=0; $expIdx < scalar(@$expVers); $expIdx++) {
         for (my $actIdx=0; $actIdx < scalar(@$actVers); $actIdx++) {
            if (@$expVers[$expIdx] eq @$actVers[$actIdx]) {
               @$expMatched[$expIdx] = 1;   # check them off...
               @$actMatched[$actIdx] = 1;
               next;
            }
         }
      }
      # all exact matches are done.
      # go through all the miss matched and find closest matches 
      # print errors and check them off...
      for (my $expIdx=0; $expIdx < scalar(@$expVers); $expIdx++) {
         next if (@$expMatched[$expIdx]);
         my $exp = @$expVers[$expIdx];
         my $actClosest = undef;
         my $clIdx = 0;
         for (my $actIdx=0; $actIdx < scalar(@$actVers); $actIdx++) {
            next if (@$actMatched[$actIdx]);
            my $act = @$actVers[$actIdx];
            #my $len = min(length($exp),length($act));
            my $len = (length($exp) < length($act)) ? length($exp) : length($act);
            my $n;
            for ($n=0; $n<$len; $n++) {
               if (substr($act,$n,1) ne substr($exp,$n,1)) {
                  last;
               }
            }
            if ($clIdx < $n) {
              $actClosest = $actIdx;
              $clIdx = $n;
            }
         }
         # we definitly have an error at this point.
         if (defined $actClosest) {
            my $act = @$actVers[$actClosest];
            push(@$errs, "$node: $fwName: expected: $exp\ngot: $act");
            $errNodes->{$node} = $node;
            @$expMatched[$expIdx] = 1;   # check them off...
            @$actMatched[$actClosest] = 1;
         }
      }
      # anything not checked off are also errors.
      for (my $expIdx=0; $expIdx < scalar(@$expVers); $expIdx++) {
         next if (@$expMatched[$expIdx]);
         push(@$errs, "$node: $fwName: not-found: " . @$expVers[$expIdx]);
         $errNodes->{$node} = $node;
      }
      for (my $actIdx=0; $actIdx < scalar(@$actVers); $actIdx++) {
         next if (@$actMatched[$actIdx]);
         push(@$errs, "$node: $fwName: unexpected: " . @$actVers[$actIdx]);
         $errNodes->{$node} = $node;
      }
   }
   my @errNodesArray=sort(keys($errNodes));
   return(\@errNodesArray, $errs);
}

my $errNodes = [];
my $errs = [];
($errNodes, $errs) = checkFirmware($nodeRange);
if (scalar(@$errNodes) > 0) {
   ($errNodes, $errs) = checkFirmware($errNodes);
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


