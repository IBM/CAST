#!/usr/bin/perl
###########################################################
#     launch1.pl
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
use bbtools;

print "copy\n";
bbcmd("$TARGET_ALL copy --tag=1 --contrib=$BBALL --filelist=$ENV{STAGEOUT_FILELIST}");

print "exit\n";
