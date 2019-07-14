#!/usr/bin/perl
###########################################################
#     stagein_user.pl
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
use lib '/opt/ibm/bb/scripts';
use Cwd 'abs_path';
use bbtools;

print "*** Entering user stage-in script ***\n";

$USER_STGIN_LISTFILE = $ENV{"USER_STGIN_LISTFILE"};
$USER_PFS_PATH       = $ENV{"USER_PFS_PATH"};

if("$USER_STGIN_LISTFILE" ne "")
{
    if(!open($IN_LIST, '<', $USER_STGIN_LISTFILE))
    {
        die "Could not open input list_file $USER_STGIN_LISTFILE\n";
    }

    $PRESTAGE_TDEF = `mktemp $USER_PFS_PATH/prestage_tdef_XXXX`;
    chomp $PRESTAGE_TDEF;
    if(!open($TDEF, '>', $PRESTAGE_TDEF))
    {
        die "Could not open transfer definition file $PRESTAGE_TDEF\n";
    }

    while($LINE = <$IN_LIST>)
    {
        chomp $LINE;
        ($SOURCE, $TARGET) = split /\s+/, $LINE;
        $SOURCE =~ s/<PFS>/$USER_PFS_PATH/;
        $TARGET =~ s/<BB>/$BBPATH/;
        print "SOURCE=$SOURCE, TARGET=$TARGET\n";
        print $TDEF "$SOURCE $TARGET 0\n";
    }

    close $IN_LIST;
    close $TDEF;

    $CONTRIBS = `seq -s, 0 $#HOSTLIST_ARRAY;
    chomp $CONTRIBS;

    $result = bbcmd("$TARGET_NODE0 gethandle --tag=1 --contrib=$CONTRIBS");
    bbcmd("$TARGET_ALL_NOBCAST copy --handle=$result->{'0'}{'out'}{'transferHandle'}" . " --filelist=$PRESTAGE_TDEF");

    unlink($PRESTAGE_TDEF);
}

print "*** Exiting user stage-in script ***\n";
