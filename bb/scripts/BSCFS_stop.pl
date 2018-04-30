#!/usr/bin/perl
###########################################################
#     BSCFS_start
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
use JSON;

$::BPOSTMBOX = 122;

foreach $HOST (@HOSTLIST_ARRAY)
{
    bpost("Unmounting BSCFS on host $HOST");
    cmd("ssh $HOST \" fusermount -u $ENV{BSCFS_MNT_PATH} \" 2>&1");
}
unlink($PRE_INSTALL_LIST);

system("rm -r $ENV{BSCFS_WORK_PATH}");
