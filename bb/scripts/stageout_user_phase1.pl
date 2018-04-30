#!/usr/bin/perl
###########################################################
#     stageout_user_phase1.pl
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

print "*** Entering user stage-out phase1 script ***\n";

print "*** Exiting user stage-out phase1 script ***\n";
