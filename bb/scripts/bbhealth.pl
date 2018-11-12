#!/usr/bin/perl
###########################################################
#     bbhealth.pl
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
use Sys::Syslog;

sub isSetuid
{
    return 0 if($> > 0);
    return 0 if($< == 0);
    return 1;
}

sub setprefix
{
    my($pf) = @_;
    $outputprefix = $pf;
}

sub output
{
    my($out, $level) = @_;

    if($setupSyslog == 0)
    {
        openlog("bbhealth", "ndelay,pid", "local0");
        $setupSyslog = 1;
    }
    if(!defined $level)
    {
        $level = LOG_INFO;
    }

    foreach $line (split("\n", $out))
    {
        syslog($level, $line);
    }
}

sub setDefaults
{
    $pollrate           = 30;
    $::DEFAULT_HOSTLIST = "localhost";
    @::GETOPS           = ("pollrate=i" => \$pollrate);
}

sub window_sleep
{
    my($pollrate) = @_;
    my $curtime = time();
    my $sleeptime = $pollrate - ($curtime - (int($curtime / $pollrate)) * $pollrate);
    sleep($sleeptime);
}

BEGIN
{
    if(isSetuid())
    {
        unshift(@INC, '/opt/ibm/bb/scripts/');
    }
    else
    {
        ($dir, $fn) = $0 =~ /(\S+)\/(\S+)/;
        unshift(@INC, abs_path($dir));
    }

    setDefaults();
}

use bbtools;

$bbtools::QUIET = 1;

do
{
    $rc = monitor();
    &window_sleep(60);
}
while($rc == -1);

sub serverOffline
{
    my($primary, $backup, $oldserver, $failurecnt) = @_;

    my @order = ($primary, $backup);
    @order = reverse(@order) if($primary eq $oldserver);

    my $sequence = $failurecnt % 2;

    &failover($order[$sequence]);
}

sub monitor
{
    $result = bbcmd("getserver --connected=primary");
    return -1 if(bbgetrc($result) != 0);
    $myprimary = $result->{"out"}{"serverList"};

    $result = bbcmd("getserver --connected=backup");
    return -1 if(bbgetrc($result) != 0);
    $mybackup = $result->{"out"}{"serverList"};

    $result = bbcmd("getserver --connected=active");
    return -1 if(bbgetrc($result) != 0);
    $myserver = $result->{"out"}{"serverList"};

    $myserver = $myprimary if(($myserver eq "") || ($myserver eq "none"));

    output("BBPrimary: $myprimary");
    output("BBBackup : $mybackup");
    output("BBActive : $myserver");

    $pollcount  = 0;
    $failurecnt = 0;
    while(1)
    {
        $result = bbcmd("getserver --connected=active");
        if(bbgetrc($result) == 0)
        {
            $myactive = $result->{"out"}{"serverList"};

            if(($myactive eq "none") || ($myactive eq ""))
            {
                # failure occurred, take action

                $failurecnt++;
                output("bbProxy does not appear to be connected to any bbServer", LOG_WARNING);
                serverOffline($myprimary, $mybackup, $myserver, $failurecnt);
            }
            else
            {
                $lvl = LOG_DEBUG;
                $lvl = LOG_INFO if(($failurecnt > 0) || ($pollcount == 0));
                output("BB connection to $myactive appears to be up", $lvl);
                $failurecnt = 0;
            }
        }
        else
        {
            if($result->{"error"}{"text"} =~ /Unable to create bb.proxy connection/i)
            {
                output("Unable to create bbProxy connection.  Attempting restart of bbProxy");
                cmd("systemctl start bbproxy.service");
            }
        }
        $pollcount++;
        &window_sleep($pollrate);
    }
}

sub failover
{
    my($newserver) = @_;

    output("Attempting failover to $newserver");

##############################################
###  Open and activate new bbServer connection

    $result = bbcmd("setserver --open=$newserver");
    return -1 if(bbgetrc($result) != 0);

    $result = bbcmd("setserver --activate=$newserver");
    return -1 if(bbgetrc($result) != 0);

    $result = bbcmd("suspend");
    return -1 if(bbgetrc($result) != 0);

##########################
###  Switch over transfers

    $result = bbcmd("adminfailover --resume=1");
    return -1 if(bbgetrc($result) != 0);
}
