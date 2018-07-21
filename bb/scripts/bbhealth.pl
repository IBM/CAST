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

sub isSetuid
{
    return 0 if($> > 0);
    return 0 if($< == 0);
    return 1;
}
sub setDefaults
{
    $newserver = "primary";
    $pollrate  = 5;
    $::DEFAULT_HOSTLIST  = "localhost";
    @::GETOPS=(
	"server=s"   => \$newserver,
	"pollrate=i" => \$pollrate
	);
}
sub window_sleep
{
    my($pollrate) = @_;
    my $curtime = time();
    my $sleeptime = $pollrate - ($curtime - (int($curtime/$pollrate)) * $pollrate);
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
	($dir,$fn) = $0 =~ /(\S+)\/(\S+)/;
	unshift(@INC, abs_path($dir));
    }
    
    setDefaults();
}

use bbtools;

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
    @order = reverse(@order) if ($primary eq $oldserver);
    
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
    
    print "BBPrimary: $myprimary\n";
    print "BBBackup : $mybackup\n";
    print "BBActive : $myserver\n";
    
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
		serverOffline($myprimary, $mybackup, $myserver, $failurecnt);
	    }
	    else
	    {
		print "BB connection to $myactive appears to be up\n";
		$failurecnt = 0;
	    }
	}
	else
	{
	    if($result->{"error"}{"text"} =~ /Unable to create bb.proxy connection/i)
	    {
		cmd("systemctl start bbproxy.service");
	    }
	}
	
	&window_sleep($pollrate);
    }
}


sub failover
{
    my($newserver) = @_;
    
    print "Attempting failover to $newserver\n";
    
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
