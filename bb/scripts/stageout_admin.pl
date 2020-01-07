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
        ($dir, $fn) = $0 =~ /(\S+)\/(\S+)/;
        unshift(@INC, abs_path($dir));
    }
}


use bbtools;
$::BPOSTMBOX = 122;
exit(0) if($::BB_SSD_MIN eq "");

@STGOUT1 = ();
push(@STGOUT1, $BB_STGOUT1_SCRIPT) if($BB_STGOUT1_SCRIPT ne "");

@STGOUT2 = ();
push(@STGOUT2, $BB_STGOUT2_SCRIPT) if($BB_STGOUT2_SCRIPT ne "");

$exitstatus = 0;

phase1() if($ARGV[0] == 1);
phase2() if($ARGV[0] == 2);
phase3() if($ARGV[0] == 3);
phase4() if($ARGV[0] == 4);

exit($exitstatus);


sub phase1
{
    &bbwaitBBServerUp();
    my $timeout = 600;
    $timeout = $jsoncfg->{"bb"}{"scripts"}{"stageout1timeout"} if(exists $jsoncfg->{"bb"}{"scripts"}{"stageout1timeout"});

    $BADEXITRC = $BADNONRECOVEXITRC;
    &setupUserEnvironment();
    push(@STGOUT1, "$bbtools::FLOOR/bb/scripts/stageout_user_phase1_bscfs.pl") if(exists $ENV{"BSCFS_MNT_PATH"});

    foreach $bbscript (@STGOUT1)
    {
        bpost("BB: Calling the user stage-out phase1 script: $bbscript");
        $rc = cmd("$bbscript", $timeout, 1);
        bpost("BB: User stage-out phase1 script completed with rc $rc.", $::BPOSTMBOX + 1, $::LASTOUTPUT);
    }
}

sub phase2
{
    $result = &bbwaitTransfersComplete();
    if($result->{"rc"})
        {   $rc=$result->{"rc"};
            bpost("BB: Stage-out phase2 waitTransfersComplete() completed with rc $rc.", $::BPOSTMBOX + 1, $::LASTOUTPUT);
        }
}

sub phase3
{
    $BADEXITRC = $BADNONRECOVEXITRC;
    &setupUserEnvironment();

    my $timeout = 600;
    $timeout = $jsoncfg->{"bb"}{"scripts"}{"stageout2timeout"} if(exists $jsoncfg->{"bb"}{"scripts"}{"stageout2timeout"});
    push(@STGOUT2, "$bbtools::FLOOR/bb/scripts/stageout_user_phase2_bscfs.pl") if(exists $ENV{"BSCFS_MNT_PATH"});
    
    foreach $bbscript (@STGOUT2)
    {
        bpost("BB: Calling the user stage-out phase2 script: $bbscript");
        $rc = cmd("$bbscript", $timeout, 1);
        bpost("BB: User stage-out phase2 script completed with rc $rc.", $::BPOSTMBOX + 2, $::LASTOUTPUT);
    }
}

sub phase4
{
    my $jobstatuscmd = $ENV{"LSF_BINDIR"} . "/bjobs -o stat -noheader $::JOBID";
    print "Command: $jobstatuscmd\n";
    my $jobstatus = `$jobstatuscmd`;
    chomp($jobstatus);
    bpost("BB: Job status=$jobstatus.  Allocation ID=" . $ENV{"CSM_ALLOCATION_ID"} . 
        "  Stagein Status=" . $ENV{"LSF_STAGE_IN_STATUS"} . 
        "  JobExit=" . $ENV{"LSB_JOBEXIT_STAT"} . 
        "  Stage Status=" . $ENV{"LSF_STAGE_JOB_STATUS"});
    
    bpost("BB: Removing logical volume $BBPATH and metadata");
    
    print "Removing logical volume $BBPATH\n";
    $result = bbcmd("$TARGET_ALL remove --mount=$BBPATH");
    if(bbgetrc($result) != 0)
    {
        bpost("BB: Remove logical volume on mount $BBPATH failed with rc " . bbgetrc($result));
    }

    print "Removing directory $BBPATH\n";
    $result = bbcmd("$TARGET_ALL rmdir --path=$BBPATH");
    if(bbgetrc($result) != 0)
    {
        bpost("BB: Remove directory $BBPATH failed with rc " . bbgetrc($result));
    }

    # Check for failures before removing metadata
    $result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBPARTIALSUCCESS");
    if(bbgetrc($result) != 0)
    {
        bpost("BB: Get Transfers failed with rc " . bbgetrc($result));
    }
    else
    {
        $numfailed = $result->{"0"}{"out"}{"numavailhandles"};
        if($numfailed > 0)
        {
            bpost("BB: Transfer(s) marked in BBPARTIALSUCCESS (failed) state");
            $exitstatus = 1;
        }
    }
    
    print "Removing job metadata\n";
    $result = bbcmd("$TARGET_NODE0 removejobinfo");
    if(bbgetrc($result) != 0)
    {
        bpost("BB: Remove job metadata failed with rc " . bbgetrc($result));
    }
    
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
