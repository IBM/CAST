#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/chk-noping/chk-noping.pm
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

my $help=0;
my $verbose=0;
my $test=$0;
$test =~ s{.*/}{};      # removes path  
$test =~ s{\.[^.]+$}{}; # removes extension


sub usage {
   print "$0 [options] noderange\n";
   print "    options\n";
   print "    -v|--verbose: set verbose on\n";
   print "    -h|--help : print this\n" ;
   exit 0
}

sub createFpingFile {
   my ($tfile, $niclist) = @_;

   open (TFILE, ">$tfile") || die "Cannot open temporary file: $tfile";
   print TFILE <<"EOF";
#!/bin/bash
fping  <<ENDFILE 2>&1 | grep -v alive
EOF

#print "niclist=".Dumper($niclist);
for my $n (@$niclist) {
   print TFILE "$n\n";
   
}
print TFILE <<"EOF";
ENDFILE
EOF
   close TFILE;
   chmod 0700,$tfile;

   if ($verbose) {
      print "$tfile=\n";
      system("cat $tfile");
   }
}

sub execFping {
   my ($cmd,$errors) = @_;
   #if ($verbose) {print $cmd . "\n";}
   print $cmd . "\n";
   my $rval = `$cmd`;
   my $rc=$?;
   if ($rc !=0 ) {
      print "(ERROR) failure in $cmd\n";
      print "(ERROR) retval = $rval";
      exit 1;
   }
   
   my $errcnt = 0;
   foreach my $l (split(/\n/,$rval)) {
      # parse off:
      push (@$errors, $l);
      $errcnt += 1;
   
   }
   return($errcnt);
}


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

my $errors=[];
my $tempdir = tempdir( CLEANUP => 1 );

my $cmd = "lsdef $nodeRange -c -i status 2>$tempdir/stderr";
print $cmd . "\n";
my $rval = `$cmd`;
my $rc=$?;
if ($rc !=0 ) {
   push(@$errors,"(ERROR) in cmd rc=$rc\n");
}
if ($verbose) {print "output: $rval";}

# all stderr are errors for this test.
foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
   chomp $l;
   push(@$errors,"$l");
}
# looking for entries like this
#  c650f99p04: status=booted

my $nodes=[];
foreach my $l (split(/\n/,$rval)) {
   my ($name,$attr,$value) = $l =~ /(\S+): (\S+)=(.*)$/;
   if (! ((defined $name) && (defined $attr) && (defined $value))) {
       print "skipping unrecognized value: $l\n";
       next;
   }
   if ( $value eq "booted" ) {
      push(@$nodes,$name);
   }
}
# now that we have all the nodes that are booted from lsdef
if ( scalar(@$nodes) > 0) { 
   my $newnoderange=join( ',', @$nodes );
   $cmd = "nodestat $newnoderange 2>$tempdir/stderr";
   print $cmd . "\n";
   my $rval = `$cmd`;
   if ($verbose) {print "output: $rval";}
   my $rc=$?;
   if ($rc !=0 ) {
      push(@$errors,"(ERROR) in cmd rc=$rc\n");
   }

   # all stderr are errors for this test.
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
      chomp $l;
      push(@$errors,"$l");
   }
   my $fpingnodes=[];
   foreach my $l (split(/\n/,$rval)) {
      my ($node,$status) = $l =~ /^(\S+): (\S+).*/;
      if ($status eq 'noping') {
         push(@$fpingnodes,$node);
      }  
      else {
         print "skipping $node, status: $status.\n" if ($verbose);
      }
   }
   if ( scalar(@$fpingnodes) > 0) { 
      my $tfile="$tempdir/fping.sh";
      createFpingFile($tfile, $nodes);

      # a bit of a lie, but we issued the nodestat when we got all the node connect info above
      # confirms the node range...
      print("nodestat $nodeRange\n");
      execFping("sudo $tfile | sort", $errors);
   }   
} else {
  print "nodes in booted file not found\n";
}

if (scalar(@$errors) > 0) {
   for my $e (@$errors) {
      print("(ERROR) $e\n");
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
}


print "$test test PASS, rc=0\n";
exit 0;

