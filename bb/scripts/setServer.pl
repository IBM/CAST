#!/usr/bin/perl
###########################################################
#     setServer.pl
#
#     Copyright IBM Corporation 2018,2018. All Rights Reserved
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
use Getopt::Long;

sub isSetuid
{
    return 0 if($> > 0);
    return 0 if($< == 0);
    return 1;
}
sub setDefaults
{
    $drain = 0;
    $newserver = "primary";
    @::GETOPS=(
	"server=s" => \$newserver,
	"drain!" => \$drain
	);
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

    setDefaults();
}

use bbtools;


my @cleanup = ();
sub failureCleanAndExit()
{
    bpost("BB setServer failure detected, cleaning up");
    foreach $cmd (reverse @cleanup)
    {
	print("BB setServer failure cleanup: $cmd\n");
        bbcmd($cmd);
    }
    bpost("BB setServer failure cleanup complete, exiting with $::BADEXITRC");
    exit($::BADEXITRC);
}

########################################
###  Retrieve current server connections

$result = bbcmd("$TARGET_ALL getserver --connected=active");
failureCleanAndExit() if(bbgetrc($result) != 0);

for($rank=0; $rank<$#::HOSTLIST_ARRAY+1; $rank++)
{
    $oldservers{$rank} = $result->{$rank}{"out"}{"serverList"};
}

if($drain == 0)
{
    $result = bbcmd("$TARGET_ALL getserver --connected=$newserver");
    failureCleanAndExit() if(bbgetrc($result) != 0);

    for($rank=0; $rank<$#::HOSTLIST_ARRAY+1; $rank++)
    {
	if($oldservers{$rank} ne $result->{$rank}{"out"}{"serverList"})
	{
	    push(@RANKS, $rank);
	}
    }
}
else
{
    my $drainserver = $newserver;

    $primary_result = bbcmd("$TARGET_ALL getserver --connected=primary");
    failureCleanAndExit() if(bbgetrc($primary_result) != 0);

    $backup_result = bbcmd("$TARGET_ALL getserver --connected=backup");
    failureCleanAndExit() if(bbgetrc($backup_result) != 0);

    for($rank=0; $rank<$#::HOSTLIST_ARRAY+1; $rank++)
    {
        if($primary_result->{$rank}{"out"}{"serverList"} eq $drainserver)
        {
            push(@RANKS, $rank);
	    $newserver = $backup_result->{$rank}{"out"}{"serverList"};
        }
	elsif($backup_result->{$rank}{"out"}{"serverList"} eq $drainserver)
	{
	    push(@RANKS, $rank);
	    $newserver = $primary_result->{$rank}{"out"}{"serverList"};
	}
    }
    if($newserver eq "none")
    {
	bpost("Node using $drainserver do not have a backup defined");
	exit($::BADEXITRC);
    }
}

if($#RANKS+1 == 0)
{
    bpost("All bbProxy connections are already on $newserver");
    exit(0);
}

$::TARGET_FAILOVER = "--jobstepid=1 --hostlist=$::HOSTLIST --target=" . join(',', @RANKS);


##############################################
###  Open and activate new bbServer connection

$result = bbcmd("$TARGET_FAILOVER setserver --open=$newserver");
failureCleanAndExit() if(bbgetrc($result) != 0);
push(@cleanup, "$TARGET_FAILOVER setserver --close=$newserver");

$result = bbcmd("$TARGET_FAILOVER setserver --activate=$newserver");
failureCleanAndExit() if(bbgetrc($result) != 0);

##############################################
###  Suspend the new active bbServer connection

$result = bbcmd("$TARGET_FAILOVER suspend");
failureCleanAndExit() if(bbgetrc($result) != 0);
push(@cleanup, "$TARGET_FAILOVER resume");

foreach $rank (@RANKS)
{
    # could do this more efficiently by grouping same oldservers
    push(@cleanup, "--jobstepid=1 --hostlist=$::HOSTLIST --target=$rank setserver --activate=$oldservers{$rank}");
}

##########################
###  Switch over transfers

$result = bbcmd("$TARGET_FAILOVER adminfailover");
failureCleanAndExit() if(bbgetrc($result) != 0);

$result = bbcmd("$TARGET_FAILOVER resume");
failureCleanAndExit() if(bbgetrc($result) != 0);


###################################
###  Close old server connection(s)
foreach $rank (@RANKS)
{
    my $count;
    for($iteration=0; $iteration < 6; $iteration++)
    {
	$result = bbcmd("--jobstepid=1 --hostlist=$::HOSTLIST --target=$rank getserver --waitforreplycount=$oldservers{$rank}");
	$count = $result->{$rank}{"out"}{"waitforreplycount"};
	last if($count == 0);
	print "Waiting for replyCount rank $rank, count=$count.  Sleeping iteration $iteration.\n";
	sleep(10);
    }

    # could do this more efficiently by grouping same oldservers
    bbcmd("--jobstepid=1 --hostlist=$::HOSTLIST --target=$rank setserver --close=$oldservers{$rank}");

    # ignore failures
}


exit(0);
