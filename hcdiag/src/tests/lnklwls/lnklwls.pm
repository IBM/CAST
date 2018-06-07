#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/lnklwls/lnklwls.pm
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
#==============================================================================
# Verify link width and speed

use strict;
use warnings;

use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use File::Temp qw/ tempdir /;
use ClustConf;
use YAML;

# SETUP: usage ----------------------------------------------------------------
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

# Get expected values ---------------------------------------------------------
my $Clustconf = new ClustConf;
$Clustconf->load();

my $node = `hostname -s`;
$node =~ s/\R//g; 
my $errs = [];
my $rc = 0;
my ($exp_lw, $exp_ls);
my $tempdir = tempdir( CLEANUP => 1 );

my $nodeCfg = $Clustconf->findNodeCfg($node);
if (defined $nodeCfg) {
    $exp_lw = $nodeCfg->{ib}->{link_width};
    if (! defined $exp_lw) {
        push(@$errs, "$node: nodeCfg->{ib}->{link_width} failed");
        $rc = 1;
    }
    $exp_ls = $nodeCfg->{ib}->{link_speed};
    if (! defined $exp_ls) {
        push(@$errs, "$node: nodeCfg->{ib}->{link_speed} failed");
        $rc = 1;
    }
}

# Use ibdiagnet to perform link width and speed check -------------------------
if ($rc == 0) {
    my $cmd = "sudo /usr/bin/ibdiagnet --pc --lw $exp_lw --ls $exp_ls " .
              "--skip dup_guids,dup_node_desc,lids,sm,nodes_info,pkey,aguid," .
              "vs_cap_smp,vs_cap_gmp,links,pm,temp_sensing " .
              "--screen_num_errs 1000 -o $tempdir/ibdiagnet 2>$tempdir/stderr";
    if ($verbose) {print "command: $cmd\n";}
    my $rval = `$cmd`;

    if (`wc -l < $tempdir/stderr` == 0) {
        # Parse the log file for link width/speed infoe
        my $info = `cat $tempdir/ibdiagnet/ibdiagnet2.log | grep "\\-I\\- Speed \\/ Width checks"`;
        $info =~ s/-I-\s//;
        my ($stage, $warn_cnt, $err_cnt) = split(/ {2,}/, $info);

        if ($err_cnt ne 0) {
            my $search = quotemeta($stage);
            my $report = `sed -n '/$search/,/Summary/{p; /^Summary/q}' $tempdir/ibdiagnet/ibdiagnet2.log`;
            $report =~ s/(?:.*\n){1,3}\z//;
            push(@$errs, "Link width/speed error(s). See report:\n\n$report");
        }
    } else {    # Error executing ibdiagnet command
        foreach my $l (split(/\n/,`cat $tempdir/stderr`)) { 
            chomp $l; 
            push(@$errs,"WARN: $l");
        }
    }
}

# Print out errors ------------------------------------------------------------
if (scalar @$errs) {
    for my $e (@$errs) {
        print "(ERROR) $e\n";
    }
    print "$test test FAIL, rc=1\n";
    exit 1;
}
print "$test test PASS, rc=0\n";
exit 0;
