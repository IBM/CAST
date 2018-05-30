#!/usr/bin/perl
#==============================================================================
#   
#    hcdiag/src/tests/chk-nvme/chk-nvme.pm
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
# Check firmware revision and vendor of NVMe devices

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
    print "    -n|--no_json: perl-JSON not installed\n";
    print "    -v|--verbose: set verbose on\n";
    print "    -h|--help : print this\n" ;
    exit 0
}

my $help=0;
my $verbose=0;
my $no_json=0;
my $test=$0;
$test =~ s{.*/}{};      # removes path  
$test =~ s{\.[^.]+$}{}; # removes extension

GetOptions(
    'h|help'      => \$help,
    'v|verbose'   => \$verbose,
    'n|no_json'   => \$no_json,
);
if ($help) { usage() }


# Get expected values ---------------------------------------------------------
my $Clustconf = new ClustConf;
$Clustconf->load();

my $node = `hostname -s`;
$node =~ s/\R//g; 
my $rc = 0;
my $errs = [];
my $exp_vendor;
my $exp_firmware;

my $nodeCfg = $Clustconf->findNodeCfg($node);
if (defined $nodeCfg) {
    $exp_vendor = $nodeCfg->{nvme}->{vendor};
    if (! defined $exp_vendor) {
        push(@$errs, "$node: nodeCfg->{nvme}->{vendor} failed");
        $rc = 1;
    }
    $exp_firmware = $nodeCfg->{nvme}->{firmware_rev};
    if (! defined $exp_firmware) {
        push(@$errs, "$node: nodeCfg->{nvme}->{firmware_rev} failed");
        $rc = 1;
    }
}

# Get device information from node and compare values -------------------------
my $tempdir = tempdir( CLEANUP => 1 );
my $firmware;
my $vendor;

if ($rc == 0) {
    if($no_json) {
        my $cmd = "lspci | grep 'Non-Volatile memory' 2>$tempdir/stderr";
        if ($verbose) {print "command: $cmd\n";}
        $vendor = `$cmd`;
        chomp $vendor;
        
        if(`cat $tempdir/stderr | grep "command not found" | wc -l` ne 0) {
            push(@$errs, "$node: lscpi command not found");
        }
        elsif (length($vendor) == 0) {
           push (@$errs, "(ERROR) Found no NVMe devices mounted");
        } else {
           # Get bus and device number of nvme device
           my $bus = substr($vendor, 0, index($vendor, ':'));
           my $busdevice = substr($vendor, 0, index($vendor, '.'));

           # Get device firmware info from sysfs using bus & device number
           my $firmcmd = "cat /sys/devices/pci$bus*/$bus*/$busdevice*/" 
                         . "nvme/nvme0/firmware_rev";
           if ($verbose) {print "command: $firmcmd\n";}
           $firmware = `$firmcmd`;
           chomp $firmware;
        }
    } else {
        use JSON;
      
        # Get device info 
        my $cmd = "sudo nvme list -o json 2>$tempdir/stderr";
        if ($verbose) {print "command: $cmd\n";}
        my $var1 = `$cmd`;
        
        if(`cat $tempdir/stderr | grep "command not found" | wc -l` ne 0) {
            push(@$errs, "$node: nvme command not found");
        } else {
            my $json = decode_json($var1);
            my $info = @{$json->{'Devices'}}[0];
            $firmware = $info->{'Firmware'};
            $vendor = $info->{'ProductName'};
        }
    }

    if (! defined $firmware || ! defined $vendor) {
        push(@$errs, "$node: firmware/vendor query failed");
    } else {
        # Compare expected and device values
        if ((index(lc($vendor), lc($exp_vendor)) == -1) || ($firmware ne $exp_firmware)) {
            push (@$errs, "$node: invalid vendor, release. \n" .
                "Expected: $exp_vendor, $exp_firmware\n" .
                "Device Product Name: $vendor\n" .
                "Device Firmware Level: $firmware\n");
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
