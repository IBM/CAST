#!/usr/bin/perl
###########################################################
#     esub.bb
#
#     Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

open(TMP, $ENV{LSB_SUB_PARM_FILE});
while($line = <TMP>)
{
    chomp($line);
    ($var,$value) = $line =~ /(\S+?)=(\S+)/;
    $ENV{$var} = $value;
}
close(TMP);

@vars = ("all");
@vars = split(",", $ENV{LSF_SUB4_SUB_ENV_VARS}) if(exists $ENV{LSF_SUB4_SUB_ENV_VARS});

open(TMP, ">/tmp/epsub_env_vars." . $ENV{LSB_SUB_JOB_ID});
foreach $var (@vars)
{
    if($var =~ /all/)
    {
	foreach $key (keys %ENV)
	{
	    print TMP "$key=$ENV{$key}\n";
	}
    }
    else
    {
	($key,$value) = $var =~ /(\S+?)=(\S+)/;
	print TMP "$key=$value\n";
    }
}
close(TMP);
