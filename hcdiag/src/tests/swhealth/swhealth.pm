#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/swhealth/swhealth.pm
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
# Switch health check

use strict;
use warnings;

use JSON;
use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use ClustConf;
use File::Temp qw/ tempdir /;
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
$test =~ s{\.[^.]+$}{}; # removes extensions

GetOptions(
    'h|help'      => \$help,
    'v|verbose'   => \$verbose,
);

if ($help) { usage() }

# Get UFM information ---------------------------------------------------------
my $Clustconf = new ClustConf;
$Clustconf->load();

my $node = `hostname -s`;
$node =~ s/\R//g;
my $errs=[];  
my $rc=0;
my ($ip, $user, $pw);

my $nodeCfg = $Clustconf->findNodeCfg($node);
if (! defined $nodeCfg) {
    push (@$errs, "$node: Clustconf->findNodeCfg($node) failed.");
    $rc = 1;
} else {
    $ip = $nodeCfg->{ufm}->{ip_address};	
    if (! defined $ip) {
        push(@$errs, "$node: nodeCfg->{ufm}->{ip_address} failed");
        $rc = 1;
    }
    $user = $nodeCfg->{ufm}->{user};    
    if (! defined $user) {
        push(@$errs, "$node: nodeCfg->{ufm}->{user} failed");
        $rc = 1;
    }
    $pw = $nodeCfg->{ufm}->{password};    
    if (! defined $pw) {
        push(@$errs, "$node: nodeCfg->{ufm}->{password} failed");
        $rc = 1;
    }
}

# Health check ----------------------------------------------------------------
my $tempdir = tempdir( CLEANUP => 1 );

if ($rc == 0) {
    # Get available switch IPs
    my $sw_cmd = "curl -u $user:$pw -X GET \"http://$ip/ufmRest/resources/systems?type=switch\" 2>$tempdir/stderr";
    if ($verbose) {print "Get switches command: $sw_cmd\n";}
    my $sw_rval = `$sw_cmd`;

    my $sw_info = decode_json($sw_rval);

    # For each switch, run health report
    foreach my $switch (@{$sw_info}) {
        if (! defined $switch->{"ip"}) {
            push(@$errs, "$node: JSON parse failure--switch IP");
        } else {
            my $sw_addr = $switch->{"ip"};
            my $sw_name = $switch->{"system_name"};

            if($sw_addr ne "0.0.0.0") {
                # Run health report for the current switch
                my $run_cmd = "curl -u $user:$pw -X POST \"http://$ip/ufmRest/actions/provisioning/Show-Health-Report\"".
                              " -d '{\"identifier\":\"ip\", \"params\":{\"arguments\":{}}, \"description\":\"\", ".
                              "\"object_ids\":[\"$sw_addr\"], \"object_type\":\"System\"}' 2>$tempdir/stderr";
                if ($verbose) {print "Run health report command: $run_cmd\n";}
                my $run = `$run_cmd`;
                my ($job) = $run =~ /jobs\/(\d+)/;
                my $status = "Running";
               
                # Check if job is complete
                while (index($status, "Completed") == -1) {
                    my $job_rval = `curl -u $user:$pw -X GET "http://$ip/ufmRest/jobs/$job" 2>$tempdir/stderr`;
                    my $report = decode_json($job_rval);
                    
                    $status = $report->{"Status"};

                    # Report error on switch health report
                    if ($status eq "Completed With Errors") {
                        my $err_summary = $report->{"Summary"};
                        if ($err_summary ne "") {
                            push(@$errs, "$sw_name: Error(s) in health report. $err_summary");
                        } else { 
                            push(@$errs, "$sw_name: Error(s) in health report. No details available."); 
                        }
                    }
                }
            }
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
