#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-gpfs-mount/chk-gpfs-mount.pm
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


#my $node = `hostname -s`;
#$node =~ s/\R//g;

my $gpu_mounts=$Clustconf->{config}->{gpfs_mounts};

if (! defined $gpu_mounts ) { 
   print "(ERROR) Clustconf->{config}->{gpfs_mounts} failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}
my $tempdir = tempdir( CLEANUP => 1 );
my $errcnt = 0;  

for my $md (@$gpu_mounts) {
  my $mount = $md->{mount};
  my $match = $md->{match};

  if ((! defined $mount) || (! defined $match)) {
     print "(ERROR) Clustconf->{config}->{gpfs_mounts} missing mount or match field\n";
  };

  my $cmd = "df -h $mount 2>$tempdir/stderr | sort";
  $match = quotemeta $match;
  print "command: $cmd\n";
  my $rval = `$cmd`;
  my $rc=$?;
  if ($rc !=0 ) {
     print "(ERROR) failure in $cmd\n";
     print "retval = $rval";
     print "$test test FAIL, rc=1\n";
     exit 1;
  }
  
  # parse all values here...
  # we should have some...
  foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
     chomp $l;
     print "(WARN) $l\n";
  }
  
  if (length($rval) == 0) {
     print "(ERROR) Found No gpfs devices mounted \n";
     print "$rval\n";
     print "$test test FAIL, rc=1\n";
     exit 1;
  }
  
  #print "match=$match\n";
  
  # now look for devices here..
  foreach my $l (split(/\n/,$rval)) {
     # parse off:
     # r92gpfs01        50T   37T   14T  73% /gpfs/automountdir/r92gpfs01
     # r92gpfs02       566T  136M  566T   1% /gpfs/automountdir/r92gpfs02
     next if ($l =~ /^Filesystem/);  # ignore the header line.
     my ($dev) = $l =~ /^(\S+) .*/;
     print "output: $l\n";
     if ((!defined $dev) || (! ($dev =~ /$match/))) {
        print "(ERROR) gpfs mount not found '$mount' : $l\n";
        $errcnt += 1;
     }
     else {
        print "gpfs mount '$mount' found, filesystem: $dev\n\n";
     }
  }
}
if ($errcnt > 0) {
   print "$test test FAIL, rc=1\n";
   exit 1;
}
print "$test test PASS, rc=0\n";
exit 0;


