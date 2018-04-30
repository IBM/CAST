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
	($dir,$fn) = $0 =~ /(\S+)\/(\S+)/;
	unshift(@INC, abs_path($dir));
    }
}

use bbtools;
$::BPOSTMBOX = 120;
exit(0) if($::BB_SSD_MIN eq "");

my @cleanup = ();
sub failureCleanAndExit()
{
    bpost("BB stage-in failure detected, cleaning up");
    foreach $cmd (reverse @cleanup)
    {
	print("BB stage-in failure cleanup: $cmd\n");
	bbcmd($cmd);
    }
    bpost("BB stage-in failure cleanup complete, exiting with $::BADEXITRC");
    exit($::BADEXITRC);
}

@STGIN = ();
push(@STGIN, "$bbtools::FLOOR/bb/scripts/stagein_user_bscfs.pl") if(exists $ENV{"BSCFS_MNT_PATH"});
push(@STGIN, $BB_STGIN_SCRIPT) if($BB_STGIN_SCRIPT ne "");

phase1() if($ARGV[0] == 1);
phase2() if($ARGV[0] == 2);
phase3() if($ARGV[0] == 3);

sub phase1()
{
    bpost("BB: Configuring LV with size $BB_SSD_MIN");
    
    print "Creating mount point $BBPATH\n";
    $result = bbcmd("$TARGET_ALL mkdir --path=$BBPATH");
    failureCleanAndExit() if(bbgetrc($result) != 0);
    push(@cleanup, "$TARGET_ALL rmdir --path=$BBPATH");
    
    print "Changing mount point $BBPATH ownership to $JOBUSER\n";
    $result = bbcmd("$TARGET_ALL chown --path=$BBPATH --user=$JOBUSER --group=$JOBGROUP");
    failureCleanAndExit() if(bbgetrc($result) != 0);
    
    print "Changing mode for mount point $BBPATH\n";
    $result = bbcmd("$TARGET_ALL chmod --path=$BBPATH --mode=0755");
    failureCleanAndExit() if(bbgetrc($result) != 0);
    
    print "Creating logical volume $BBPATH with size $BB_SSD_MIN\n";
    $result = bbcmd("$TARGET_ALL create --mount=$BBPATH --size=$BB_SSD_MIN");
    failureCleanAndExit() if(bbgetrc($result) != 0);
    push(@cleanup, "$TARGET_ALL remove --mount=$BBPATH");
    
    print "Changing logical volume $BBPATH ownership to $JOBUSER\n";
    $result = bbcmd("$TARGET_ALL chown --path=$BBPATH --user=$JOBUSER --group=$JOBGROUP");
    failureCleanAndExit() if(bbgetrc($result) != 0);
    
    print "Changing mode for logical volume $BBPATH\n";
    $result = bbcmd("$TARGET_ALL chmod --path=$BBPATH --mode=0755");
    failureCleanAndExit() if(bbgetrc($result) != 0);
}

sub phase2
{    
    $BADEXITRC = $BADNONRECOVEXITRC;
    &setupUserEnvironment();
    foreach $bbscript (@STGIN)
    {
	bpost("BB: Calling user stage-in script: $bbscript");
	$rc = cmd("$bbscript");
	bpost("BB: User stage-in script exited with $rc", $::BPOSTMBOX+1);
	failureCleanAndExit() if($rc != 0);
    }
}

sub phase3
{    
    if($#STGIN >= 0)
    {
	$result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBNOTSTARTED,BBINPROGRESS");
	$numpending = $result->{"0"}{"out"}{"numavailhandles"};
	while ($numpending > 0)
	{
	    bpost("Waiting for $numpending transfer(s) to complete");
	    sleep(5);
	    $result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBINPROGRESS");
	    $numpending = $result->{"0"}{"out"}{"numavailhandles"};
	}
	
	# Check for any failed transfers and halt job execution phase if found
        $result = bbcmd("$TARGET_QUERY gettransfers --numhandles=0 --match=BBNOTSTARTED,BBFAILED");
	$numfailed = $result->{"0"}{"out"}{"numavailhandles"};
	failureCleanAndExit() if($numfailed > 0);
    }
    else
    {
	bpost("BB: No user stage-in script specified");
    }
    bpost("BB: Stage-in admin script completed");
}

exit(0);
