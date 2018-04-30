#!/usr/bin/perl
###########################################################
#     stageout_user_bscfs_phase2.pl
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
BEGIN
{
    ($dir,$fn) = $0 =~ /(\S+)\/(\S+)/;
    unshift(@INC, abs_path($dir));
}
use bbtools;

print "*** Entering BSCFS user stage-out phase2 script ***\n";

$BSCFS_WORK_PATH = $ENV{"BSCFS_WORK_PATH"};

my %CUM_STATUS;
my %CUM_COMMAND;
my %SUCCESSFUL_TRANSFERS;

$RESULT = bbcmd("$TARGET_QUERY gettransfers " .
		    "--numhandles=1000000 --match=BBFULLSUCCESS");
@SUCCESSFUL_TRANSFERS{($RESULT->{"0"}{"handles"} =~ m/(\d+)/g)} = ();

for ($NODE = 0; $NODE <= $#HOSTLIST_ARRAY; $NODE++) {
    $CLEANUP_LIST = "$BSCFS_WORK_PATH/$JOBID/cleanup_list.$NODE";
    if (open($LIST, '<', $CLEANUP_LIST)) {
	while ($LINE = <$LIST>) {
	    chomp $LINE;
	    ($HANDLE, $USER_SCRIPT, $SHARED_FILE, $MAP_FILE) =
		($LINE =~ /^\s*(\d*)\s*("[^"]*")\s*("[^"]*")\s*("[^"]*")\s*$/);
	    $STATUS = "BBFULLSUCCESS";
	    if (! exists $SUCCESSFUL_TRANSFERS{$HANDLE}) {
		$RESULT = bbcmd("$TARGET_QUERY getstatus --handle=$HANDLE");
		$STATUS = $RESULT->{"0"}{"out"}{"status"};
	    }

	    if (! exists $CUM_STATUS{$SHARED_FILE}) {
		$CUM_STATUS{$SHARED_FILE} = "BBFULLSUCCESS";
	    }
	    if ($STATUS ne "BBFULLSUCCESS") {
		$CUM_STATUS{$SHARED_FILE} = $STATUS;
	    }

	    $CUM_COMMAND{$USER_SCRIPT.' '.$SHARED_FILE.' '.$MAP_FILE} = 1;
	}

	close $LIST;
	system("rm -f $CLEANUP_LIST");
    }
}

for $COMMAND (keys %CUM_COMMAND) {
    ($USER_SCRIPT, $SHARED_FILE, $MAP_FILE) =
	($COMMAND =~ /^("[^"]*") ("[^"]*") ("[^"]*")$/);
    if ($USER_SCRIPT ne "\"\"") {
	system("$USER_SCRIPT $CUM_STATUS{$SHARED_FILE} $SHARED_FILE $MAP_FILE");
    } else {
	print "No user cleanup script specified for file $SHARED_FILE.\n";
    }
}

print "*** Exiting BSCFS user stage-out phase2 script ***\n";
