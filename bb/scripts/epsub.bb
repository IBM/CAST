#!/usr/bin/perl
###########################################################
#     epsub.bb
#
#     Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

use JSON;
use File::Temp qw/ tempfile /;

sub bbfail
{
    my($desc) = @_;
    print "$desc\n";
    die $desc;
}

sub bpost
{
    my($desc, $mailbox, $filedata) = @_;
    $bpostbin = $ENV{'LSF_BINDIR'};
    if($bpostbin ne "")    # LSF available
    {
        my $fh;
        my $tmpfilename;
        my $fileoption = "";
        $mailbox = $::BPOSTMBOX if(!defined $mailbox);
        if($filedata ne "")
        {
            ($fh, $tmpfilename) = tempfile();
            $fileoption = "-a $tmpfilename";
            print $fh $filedata;
        }
        system("$bpostbin/bpost $fileoption -d '$desc' -i $mailbox " . $ENV{"LSB_SUB_JOB_ID"});
        
        if($filedata ne "")
        {
            close($fh);
            unlink($tmpfilename);
        }
    }
    else
    {
        print "Message: $desc\n";
    }
}

# Check for job submission failure and abort
if($ENV{"LSB_SUB_JOB_ID"} eq "-1")
{
    exit(0);
}

open(TMP, $ENV{LSB_SUB_PARM_FILE});
while($line = <TMP>)
{
    chomp($line);
    ($var,$value) = $line =~ /(\S+?)=(\S+)/;
    $ENV{$var} = $value;
}
close(TMP);

@vars = ("all");
if(exists $ENV{LSF_SUB4_SUB_ENV_VARS})
{
    @vars = split(",", $ENV{LSF_SUB4_SUB_ENV_VARS});
    push(@vars, "BBPATH="         . $ENV{"BBPATH"})         if(exists $ENV{"BBPATH"});
    push(@vars, "BSCFS_MNT_PATH=" . $ENV{"BSCFS_MNT_PATH"}) if(exists $ENV{"BSCFS_MNT_PATH"});;
}

%SKIP = ( "CSM_ALLOCATION_ID" => 1);

my @ENVDATA = ();
foreach $var (@vars)
{
    if($var =~ /all/)
    {
        foreach $key (keys %ENV)
        {
            push(@ENVDATA, "$key=$ENV{$key}") if(!exists $SKIP{$key});
        }
    }
    else
    {
        ($key, $value) = $var =~ /(\S+?)=(\S+)/;
        push(@ENVDATA, "$key=$vaule");
    }
}
my $envdata_str = join("\n", @ENVDATA);
bpost("BB Path=" . $ENV{"BBPATH"}, 119, $envdata_str);
