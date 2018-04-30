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

print "randfile\n";
system("jsrun -r 1 /opt/ibm/bb/tools/randfile --sourcepath $ENV{BBPATH} --targetpath $ENV{STAGEOUT_DIR} --minfiles 1 --maxfiles 1 --minsize 1 --maxsize 134217728 --genfilelist --by 512 --file $ENV{STAGEOUT_FILELIST}");

print "md5sum\n";
system("jsrun -r 1 -h $ENV{BBPATH} /u/tgooding/bluecoral/bb/tests/src/md5sum_cwd.sh > $ENV{STAGEOUT_MD5SUM}");

print "exit\n";
