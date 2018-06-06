#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-sys-firmware/chk-sys-firwmare-local.pm
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
use File::Temp qw/ tempdir /;
use List::MoreUtils 'first_index'; 
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

my $node = `hostname -s`;

my $err=0;
my $tempdir = tempdir( CLEANUP => 1 );
my $sysfsdir = "/sys/firmware/devicetree/base/ibm,firmware-versions";

###/sys/firmware/devicetree/base/ibm,firmware-versions
# check the firmware,
# return a list of nodes in error
# and the error text to print.
sub checkFirmware {
   my $Clustconf = new ClustConf;
   $Clustconf->load();
   my $nodeCfg = $Clustconf->findNodeCfg($node);
   if ( !defined $nodeCfg ) {
      print "$node: Clustconf->findNodeCfg($node) failed\n";
      print "$test test FAIL, rc=1\n";
      exit 1;
   }

   my $errs = [];
   my $fwName = $nodeCfg->{firmware}->{name}; # release name
   if (! defined $fwName ) {
      push(@$errs, "cannot find firmware->name in $node yaml file");
   }
   my $expVers = $nodeCfg->{firmware}->{versions};
   if ( ! defined $expVers ) {
      push(@$errs, "cannot find firmware->versions in $node yaml file");
      return($errs);
   }
   if ( $verbose ) { 
      print "Expected versions:\n";
      print join("\n", @$expVers); 
      print "\n\n";
   };

   # we are good to proceed
   foreach my $file (`ls $sysfsdir`) { 
      chomp $file;
      if ( $file ne "name"  && $file ne "phandle" ) {
         my $index = first_index { /$file/ } @$expVers;
         my $curr = `cat $sysfsdir/$file`;
         chop $curr;
         if ( defined $index && ( $index >= 0) ) { 
            my $exp= @$expVers[$index];
            if ( $exp =~ /$curr$/) { 
               if ( $verbose ) { print "Version match: $exp.\n"; };
            }
            else { 
               push(@$errs, "Expected: $exp, got: $curr.");
            }
         }
         else {
            print "(WARN) `$file` found under $sysfsdir, with content: $curr, but not defined in yaml file\n";
         }
      }
   }
   return($errs);
} 

my $errs = [];
($errs) = checkFirmware($node);


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


