#!/usr/bin/perl
###########################################################
#     stagein_admin.pl
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
    print "argv[0] = $ARGV[0]\n";
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
$::BPOSTMBOX = 120;
exit(0) if($::BB_SSD_MIN eq "");

my @cleanup = ();
sub failureCleanAndExit
{
    my ($reason) = @_;
    bpost("BB stage-in failure detected: $reason");
    foreach $cmd (reverse @cleanup)
    {
        print("BB stage-in failure cleanup: $cmd\n");
        bbcmd($cmd);
    }
    bpost("BB stage-in failure cleanup complete, exiting with $::BADEXITRC");
    exit($::BADEXITRC);
}

@STGIN = ();
push(@STGIN, $BB_STGIN_SCRIPT) if($BB_STGIN_SCRIPT ne "");

phase1() if($ARGV[0] == 1);
phase2() if($ARGV[0] == 2);
phase3() if($ARGV[0] == 3);

sub phase1()
{
    bpost("BB: Configuring LV with size $BB_SSD_MIN");
    
    print "Creating mount point $BBPATH\n";
    $result = bbcmd("$TARGET_ALL mkdir --path=$BBPATH");
    push(@cleanup, "$TARGET_ALL rmdir --path=$BBPATH") if(bbgetsuccess($result) > 0);
    failureCleanAndExit("mkdir failed") if(bbgetrc($result) != 0);
    
    print "Changing mount point $BBPATH ownership to $JOBUSER\n";
    $result = bbcmd("$TARGET_ALL chown --path=$BBPATH --user=$JOBUSER --group=$JOBGROUP");
    failureCleanAndExit("chown failed") if(bbgetrc($result) != 0);
    
    print "Changing mode for mount point $BBPATH\n";
    $result = bbcmd("$TARGET_ALL chmod --path=$BBPATH --mode=0750");
    failureCleanAndExit("chmod failed") if(bbgetrc($result) != 0);
    
    print "Creating logical volume $BBPATH with size $BB_SSD_MIN\n";
    $result = bbcmd("$TARGET_ALL create --mount=$BBPATH --size=$BB_SSD_MIN");
    push(@cleanup, "$TARGET_ALL remove --mount=$BBPATH") if(bbgetsuccess($result) > 0);
    failureCleanAndExit("create LV failed") if(bbgetrc($result) != 0);
}

sub phase2_failure
{
    open(FH,'>',$ENV{"BB_ERR_FILE"});
    close(FH);
    exit(0);
}

sub phase2
{
    $rc = &setupUserEnvironment();
    &phase2_failure() if($rc);
    my $timeout = 600;
    $timeout = $jsoncfg->{"bb"}{"scripts"}{"stageintimeout"} if(exists $jsoncfg->{"bb"}{"scripts"}{"stageintimeout"});

    unshift(@STGIN, "$bbtools::FLOOR/bb/scripts/stagein_user_bscfs.pl") if(exists $ENV{"BSCFS_MNT_PATH"});

    foreach $bbscript (@STGIN)
    {
        bpost("BB: Calling user stage-in script: $bbscript");
        $scriptrc = cmd("$bbscript 2>&1", $timeout, 1);
        bpost("BB: User stage-in script exited with $scriptrc ", $::BPOSTMBOX + 1, $::LASTOUTPUT);
        &phase2_failure() if($scriptrc != 0);
    }
}

sub phase3
{
    push(@cleanup, "$TARGET_NODE0 removejobinfo");
    push(@cleanup, "$TARGET_ALL rmdir --path=$BBPATH");
    push(@cleanup, "$TARGET_ALL remove --mount=$BBPATH");
    
    if($#STGIN >= 0)
    {
        &bbwaitTransfersComplete();
        
        # Check for any failed transfers and halt job execution phase if found
        $result    = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBNOTSTARTED,BBINPROGRESS,BBPARTIALSUCCESS");
        $numfailed = $result->{"0"}{"out"}{"numavailhandles"};
        if($numfailed > 0)
        {
            # Handles that haven't successfully started as user script errors.  
            # Failed or stuck-in-progress are potentially recoverable.
            $result    = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBNOTSTARTED");
            $numnotstarted = $result->{"0"}{"out"}{"numavailhandles"};
            if($numnotstarted == $numfailed)
            {
                $BADEXITRC = $BADNONRECOVEXITRC;
                &failureCleanAndExit("User script exited without starting a transfer on $numnotstarted handle(s)");
            }
            else
            {
                &failureCleanAndExit("$numfailed handles did not complete normally");
            }
        }
        if( -e $ENV{"BB_ERR_FILE"})
        {  
            unlink $ENV{"BB_ERR_FILE"};
            $BADEXITRC = $BADNONRECOVEXITRC;
            &failureCleanAndExit("User script failed");
        }
    }
    else
    {
        bpost("BB: No user stage-in script specified");
    }
    bpost("BB: Stage-in admin script completed");
}

exit(0);
