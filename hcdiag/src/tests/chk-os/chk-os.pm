#!/usr/bin/perl

#==============================================================================
##   
##    hcdiag/src/tests/chk-os/chk-os.pm
## 
##  © Copyright IBM Corporation 2015,2016. All Rights Reserved
##
##    This program is licensed under the terms of the Eclipse Public License
##    v1.0 as published by the Eclipse Foundation and available at
##    http://www.eclipse.org/legal/epl-v10.html
##
##    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
##    restricted by GSA ADP Schedule Contract with IBM Corp.
## 
##=============================================================================

use strict;
use warnings;

use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use ClustConf;
use File::Temp qw/ tempdir /;
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

my $node = `hostname -s`;
$node =~ s/\R//g; 
my $nodeCfg = $Clustconf->findNodeCfg($node);
if (!defined $nodeCfg) {
   print "$node: Clustconf->findNodeCfg($node) failed\n";
   print "$test test FAIL, rc=1\n";
   exit 1;
}

# Set commands to get os version and kernel release
my $tempdir = tempdir( CLEANUP => 1 );
my $os_cmd = "cat /etc/os-release | grep PRETTY_NAME | cut -d\= -f2 2>$tempdir/stderr";
my $kernel_cmd = "uname -r 2>$tempdir/stderr";
print $os_cmd . "\n";
print $kernel_cmd . "\n";

# Execute commands and save output
my $version = `$os_cmd`;
$version =~ s/"//g;
$version =~ s/\R//g;
my $release = `$kernel_cmd`;
$release =~ s/\R//g;

my $rc=$?;
my $errs = [];
my $err=0;

# Get expected OS version and kernel release
my $exp_os = $nodeCfg->{os}->{pretty_name};
if (! defined $exp_os) {
   push(@$errs, "$node: nodeCfg->{os}->{pretty_name} failed");
}
my $exp_kernel = $nodeCfg->{kernel}->{release};
if (! defined $exp_kernel) {
   push(@$errs, "$node: nodeCfg->{kernel}->{release} failed");
}

# Check for match
if (($version ne $exp_os) || ($release ne $exp_kernel)) {
  push (@$errs, "$node: invalid version, release: " .
                $version . ','.
                $release . '; ' .
                "expected: $exp_os, $exp_kernel");
}
else {
  print "OS version $version and kernel release $release match\n";
}

# Print out the errors with an approrpiate error header
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
}

print "$test test PASS, rc=0\n";
exit 0;

