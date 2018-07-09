#!/usr/bin/perl
###########################################################
#     stagein_user_bscfs.pl
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

print "*** Entering BSCFS user stage-in script ***\n";

$BSCFS_STGIN_LISTFILE = $ENV{"BSCFS_STGIN_LISTFILE"};
$BSCFS_MNT_PATH = $ENV{"BSCFS_MNT_PATH"};
$BSCFS_PFS_PATH = $ENV{"BSCFS_PFS_PATH"};
$BSCFS_WORK_PATH = $ENV{"BSCFS_WORK_PATH"};

# Facts copied from bb/src/bbfileflags.h ....
$BBTransferTypeMask    = 0x0030;
$BBTransferTypeShift   = 4;
$BBTransferOrderMask   = 0x00c0;
$BBTransferOrderShift  = 6;
$BBFileBundleIDMask    = 0xff00;
$BBFileBundleIDShift   = 8;
$BBTransferTypeRegular = 0;
$BBTransferTypeBSCFS   = 1;

system("mkdir -p $BSCFS_WORK_PATH/$JOBID");

print "Creating BSCFS BB directory $BSCFS_BB_PATH\n";
bbcmd("$TARGET_ALL mkdir --path=$BSCFS_BB_PATH");

if ("$BSCFS_STGIN_LISTFILE" ne "") {
    if (! open($IN_LIST, '<', $BSCFS_STGIN_LISTFILE)) {
	bbfail "Could not open input list_file $BSCFS_STGIN_LISTFILE\n";
    }

    if (! open($OUT_LIST, '>', $PRE_INSTALL_LIST)) {
	bbfail "Could not open output list_file $PRE_INSTALL_LIST\n";
    }

    $PRESTAGE_TDEF = "$BSCFS_WORK_PATH/$JOBID/prestage_tdef";
    if (! open($TDEF, '>', $PRESTAGE_TDEF)) {
	bbfail "Could not open transfer definition file $PRESTAGE_TDEF\n";
    }

    $BUNDLE_ID = 1;
    while ($LINE = <$IN_LIST>) {
	chomp $LINE;
	($SHARED_FILE, $MAP_FILE) = split /\s+/, $LINE;
	($SHARED_FILE_REL = $SHARED_FILE) =~ s/$BSCFS_PFS_PATH\///;
	($INTERNAL_BASE = $SHARED_FILE_REL) =~ s/\//,/g;

	if ($BUNDLE_ID > ($BBFileBundleIDMask >> $BBFileBundleIDShift)) {
	    warn "Too many files for BSCFS pre-stage. " .
		    "Skipping file $SHARED_FILE.\n";
	    next;
	}
	printf $TDEF
	    "$MAP_FILE $BSCFS_BB_PATH/$INTERNAL_BASE.index %d\n",
	    (($BUNDLE_ID << $BBFileBundleIDShift) |
	     (0 << $BBTransferOrderShift) |
	     ($BBTransferTypeBSCFS << $BBTransferTypeShift));
	printf $TDEF
	    "$SHARED_FILE $BSCFS_BB_PATH/$INTERNAL_BASE.data %d\n",
	    (($BUNDLE_ID << $BBFileBundleIDShift) |
	     (1 << $BBTransferOrderShift) |
	     ($BBTransferTypeBSCFS << $BBTransferTypeShift));

	print $OUT_LIST "\"$BSCFS_MNT_PATH/$SHARED_FILE_REL\" " .
			"\"$BSCFS_BB_PATH/$INTERNAL_BASE.index\" " .
			"\"$BSCFS_BB_PATH/$INTERNAL_BASE.data\"\n";

	$BUNDLE_ID++;
    }

    close $IN_LIST;
    close $OUT_LIST;
    close $TDEF;

    $CONTRIBS = `seq -s, 0 $#HOSTLIST_ARRAY`;
    chomp $CONTRIBS;

    $result = bbcmd("$TARGET_NODE0 gethandle --tag=1 --contrib=$CONTRIBS");
    bbcmd("$TARGET_ALL copy --handle=$result->{'0'}{'out'}{'transferHandle'}" .
	  " --filelist=$PRESTAGE_TDEF");

    system("rm -f $PRESTAGE_TDEF");
}

print "*** Exiting BSCFS user stage-in script ***\n";
