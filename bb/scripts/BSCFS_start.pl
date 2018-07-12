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

$::BPOSTMBOX = 120;

if(!-e $ENV{BSCFS_WORK_PATH})
{
    mkdir($ENV{BSCFS_WORK_PATH}, 0700) || bbfail "Failure to create BSCFS_WORK_PATH";
}

$PRE_INSTALL_OPTION="";
$PRE_INSTALL_OPTION = "--pre_install_list $PRE_INSTALL_LIST" if(-r $PRE_INSTALL_LIST);

$NODE=0;
$NODE_COUNT = $#HOSTLIST_ARRAY+1;
foreach $HOST (@HOSTLIST_ARRAY)
{
    bpost("Starting BSCFS on $HOST");
    
    $env = "LSB_JOBID=$JOBID";
    
    my @args = ();
    push(@args, "$bbtools::FLOOR/bscfs/agent/bscfsAgent");
    push(@args, $ENV{BSCFS_MNT_PATH});
    push(@args, "--node_count $NODE_COUNT");
    push(@args, "--node_number $NODE");
    push(@args, "--config /etc/ibm/bb.cfg");
    push(@args, "--pfs_path $ENV{BSCFS_PFS_PATH}");
    push(@args, "--bb_path $BSCFS_BB_PATH");
    push(@args, "--write_buffer_size $ENV{BSCFS_WRITE_BUFFER_SIZE}");
    push(@args, "--read_buffer_size $ENV{BSCFS_READ_BUFFER_SIZE}");
    push(@args, "--data_falloc_size $ENV{BSCFS_DATA_FALLOC_SIZE}");
    push(@args, "--max_index_size $ENV{BSCFS_MAX_INDEX_SIZE}");
    push(@args, "--cleanup_list $CLEANUP_LIST.$NODE");
    push(@args, $PRE_INSTALL_OPTION);
    $cmd = join(" ", @args);
    cmd("ssh $HOST \" $env $cmd \" 2>&1");
    $NODE++;
}
