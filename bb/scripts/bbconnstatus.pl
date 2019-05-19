#!/usr/bin/perl

use JSON;
use Cwd 'abs_path';
use Getopt::Long;

sub setDefaults
{
    $::DEFAULT_HOSTLIST = "localhost";
    @::GETOPS=(
        "v!" => \$verbose
	);
}

BEGIN
{
    unshift(@INC, '/opt/ibm/bb/scripts/');
    setDefaults();
}

use bbtools;
$bbtools::QUIET = 1 if(!$verbose);

$rp = bbcmd("getserver --connected=primary");
$rb = bbcmd("getserver --connected=backup");
$ra = bbcmd("getserver --connected=active");
if((bbgetrc($ra) != 0) || (bbgetrc($rp) != 0) || (bbgetrc($rb) != 0))
{
    print "bbProxy_might_be_down\n";
}
elsif($ra->{"out"}{"serverList"} eq $rp->{"out"}{"serverList"})
{
    print "primary\n";
}
elsif($ra->{"out"}{"serverList"} eq $rb->{"out"}{"serverList"})
{
    print "backup\n";
}
elsif($ra->{"out"}{"serverList"} eq "")
{
    print "no_connection\n";
}
else
{
    print "unknown_server=" . $ra->{"out"}{"serverList"} . "\n";
}
