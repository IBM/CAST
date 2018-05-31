#!/usr/bin/perl    
#================================================================================
#   
#    hcdiag/src/tests/chk-sw-level/chk-sw-level.pm
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
my $errs = [];
my $tempdir = tempdir( CLEANUP => 1 );

sub checkSwVersion {
   my ( $sw, $version ) = @_;
   if (( defined $sw ) && ( defined $version )) {
      my $cmd = "rpm -qa|grep $sw | grep $version 2>$tempdir/stderr";
      if ($verbose) {print "\ncommand: $cmd\n";}
      my $rval = `$cmd`;
      my $rc=$?;
      if ($verbose) {print "output: $rval";}
      foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { chomp $l; print "(WARN) $l\n"; }

      print "\nChecking $sw, $version:\n";
      if (length($rval) == 0) { 
         print "ERROR: Not installed.\n"; 
         $err++;
      } else { 
         print "$rval";
      }
   } else {
      push(@$errs,"ERROR: invalid number of parameters invoking checkSwVersion.\n"); 
   }
}


###
my $Clustconf = new ClustConf;
$Clustconf->load();
my $nodeCfg = $Clustconf->findNodeCfg($node);
if ( !defined $nodeCfg ) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}
my $swlist = $nodeCfg->{software};
#print "others".Dumper($swlist)."\n";
if (! defined $swlist ) {
   push(@$errs, "cannot find software->others in $node yaml file");
}
else {
   #print "swlist=".Dumper($swlist)."\n";
   for (my $i=0; $i < scalar(@$swlist); $i++) {
       my $k=(keys %{$swlist->[$i]})[0]; 
       my $v=$swlist->[$i]->{$k};
       #print "key is $k, value=$v\n";
       checkSwVersion( $k, $v );
   }
}


# print out the errors with an approrpiate error header...
if (scalar @$errs)  {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "\n$test test FAIL, rc=1\n";
   exit 1;
} 

if ( $err)  { 
   print "\n$test test FAIL, rc=$err\n";
   exit 1;
} 


print "\n$test test PASS, rc=0\n";
exit 0;


