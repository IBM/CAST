#!/usr/bin/perl
###########################################################
#     stageout_admin.pl
#
#     Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

use JSON;
use Cwd 'abs_path';

sub isSetuid
{
    return 0 if($> > 0);
    return 0 if($< == 0);
    return 1;
}

BEGIN
{
    if(isSetuid())
    {
	unshift(@INC, '/opt/ibm/bb/scripts/');
    }
    else
    {
	($dir,$fn) = $0 =~ /(\S+)\/(\S+)/;
	unshift(@INC, abs_path($dir));
    }
}

use bbtools;
$::BPOSTMBOX = 122;
exit(0) if($::BB_SSD_MIN eq "");

@STGOUT1 = ();
push(@STGOUT1, $BB_STGOUT1_SCRIPT) if($BB_STGOUT1_SCRIPT ne "");
push(@STGOUT1, "$bbtools::FLOOR/bb/scripts/stageout_user_phase1_bscfs.pl") if($ENV{"LSB_SUB_ADDITIONAL"} =~ /bscfs/);

@STGOUT2 = ();
push(@STGOUT2, $BB_STGOUT2_SCRIPT) if($BB_STGOUT2_SCRIPT ne "");
push(@STGOUT2, "$bbtools::FLOOR/bb/scripts/stageout_user_phase2_bscfs.pl") if($ENV{"LSB_SUB_ADDITIONAL"} =~ /bscfs/);

$exitstatus = 0;

phase1() if($ARGV[0] == 1);
phase2() if($ARGV[0] == 2);
phase3() if($ARGV[0] == 3);
phase4() if($ARGV[0] == 4);

exit($exitstatus);


sub phase1
{
    $BADEXITRC = $BADNONRECOVEXITRC;
    &setupUserEnvironment();
    foreach $bbscript (@STGOUT1)
    {
	bpost("BB: Calling the user stage-out phase1 script: $bbscript");
	$rc = cmd("$bbscript");
	bpost("BB: User stage-out phase1 script completed with rc $rc.", $::BPOSTMBOX+1);
    }
}

sub phase2
{
# NOTE: If we bring down the file system, then we should issue the following command.
#       This command tells bbserver to start sending 'progress' messages to bbproxy.
#       If we were to resize/shrink the LV, bbproxy would use these progress messages
#       to perform the resize.
    
# bbcmd("stgout_start --mount=$BBPATH");

# jobstepid=0 means to query all transfers under jobid
    $result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBNOTSTARTED,BBINPROGRESS");
    $numpending = $result->{"0"}{"out"}{"numavailhandles"};
    while ($numpending > 0)
    {
	bpost("BB: Waiting for $numpending transfer(s) to complete");
	sleep(5);
	$result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBINPROGRESS");  # All transfers should have started
	$numpending = $result->{"0"}{"out"}{"numavailhandles"};
    }
}

sub phase3
{
    $BADEXITRC = $BADNONRECOVEXITRC;
    &setupUserEnvironment();
    foreach $bbscript (@STGOUT2)
    {
	bpost("BB: Calling the user stage-out phase2 script: $bbscript");
	$rc = cmd("$bbscript");
	bpost("BB: User stage-out phase2 script completed with rc $rc.", $::BPOSTMBOX+2);
    }
}

sub phase4
{
    $jobstatus = cmd("bjobs -o stat -noheader $::JOBID");
    if(($jobstatus =~ /DONE/) ||
       ($jobstatus =~ /EXIT/) ||
       (exists $ENV{"CSM_ALLOCATION_ID"}))
    {
	my $bbenvfile = getBBENVName();
	unlink($bbenvfile);
    }
    
    bpost("BB: Removing logical volume $BBPATH and metadata");
    
    print "Removing logical volume $BBPATH\n";
    bbcmd("$TARGET_ALL remove --mount=$BBPATH");
    
    print "Removing directory $BBPATH\n";
    bbcmd("$TARGET_ALL rmdir --path=$BBPATH");

    # Check for failures before removing metadata
    $result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBFAILED");
    $numfailed = $result->{"0"}{"out"}{"numavailhandles"};
    if($numfailed > 0)
    {
	bpost("BB: Transfer(s) marked in BBFAILED state");
	$exitstatus = 1;
    }
    
    print "Removing job metadata\n";
    bbcmd("$TARGET_NODE0 removejobinfo");
    
    if($exitstatus == 0)
    {
	bpost("BB: stage-out admin script completed");
    }
    else
    {
	bpost("BB: stage-out admin script failed with exit status $exitstatus");
    }
    print "*** Exiting admin stage-out script ***\n";
}
