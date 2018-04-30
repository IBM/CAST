#!/usr/bin/perl
#
#================================================================================
#   
#    hcdiag/src/tests/chk-hca-attributes/chk-hca-attributes.sh
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

# check hca_attributes of IB adapters (fw_version, board_id)
# ignore nodes from ignore.list under common

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

my $errs=[];  
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
my $rc=0;
my $tempdir = tempdir( CLEANUP => 1 );
my $key_re='fw_ver|board_id';

my $nodeCfg = $Clustconf->findNodeCfg($node);
if ( defined $nodeCfg ) {
   my $fw_version=$nodeCfg->{ib}->{firmware};
   if (! defined $fw_version ) {
      push (@$errs, "$node: nodeCfg->{ib}->{firmware} failed");
      $rc=1;
   }
   my $board_id=$nodeCfg->{ib}->{board_id};
   if (! defined $board_id ) {
      push (@$errs, "$node: nodeCfg->{ib}->{board_id} failed");
      $rc=1;
   }
   if ( $rc == 0 ) {
      my $cmd="ibv_devinfo | egrep 'board_id|fw' 2>$tempdir/stderr";
      if ($verbose) {print "command: $cmd\n\n";}
      my @out=split(/\n/,`$cmd`);

      foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }
      
      if (scalar(@out) == 0) { push(@$errs,"Found No command output."); }
   
      foreach $_ (@out) {
         if ( /\s+($key_re):\s+(\S+)/ ) {
            my($key,$val)=($1,$2);
            #print "key is $key, value is $val\n";
            if ( defined $key ) {
               if ( defined $val ) {
                  if ( $key eq "fw_ver" ) {
                     if ( $val ne $fw_version ) {
                        push (@$errs, "$key, expecting $fw_version, got: $val");
                     }
                     else {
                        if ($verbose) {print "$key, expecting: $fw_version, got: $val\n";}
                     }
                  }
                  elsif ( $key eq "board_id" ) { 
                     if ( $val ne $board_id ) {
                         push (@$errs, "$key, expecting $board_id, got: $val");
                     }
                     else {
                        if ($verbose) {print "$key, expecting: $board_id, got: $val\n";}
                     }
                  }
                  else {
                     push (@$errs, "Invalid key: $key");
                  }
               }    
               else {
                   push (@$errs, "val not defined.");
               }
            }    
            else {
               push (@$errs, "Key not defined.");
            }
         }
      } # for
   }
}
else {  
   push (@$errs, "$node: Clustconf->findNodeCfg($node) failed.");
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

