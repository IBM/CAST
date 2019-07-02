#!/usr/bin/perl

use JSON;
use Cwd 'abs_path';
use Getopt::Long;

sub setDefaults
{
    $raw = 0;
    $::DEFAULT_HOSTLIST = "localhost";
    @::GETOPS=(
        "v!" => \$verbose,
        "raw!" => \$raw
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
    if($ra->{"error"}{"func"} =~ /checkForSuperUserPermission/i)
    {
        print "Superuser_permission_required_to_query_connection_status\n";
    }
    else
    {
        print "bbProxy_might_be_down\n";
    }
}
elsif($ra->{"out"}{"serverList"} eq $rp->{"out"}{"serverList"})
{
    my $rawtext = " (" . $ra->{"out"}{"serverList"} . ")" if($raw);
    print "primary$rawtext\n";
}
elsif($ra->{"out"}{"serverList"} eq $rb->{"out"}{"serverList"})
{
    my $rawtext = " (" . $ra->{"out"}{"serverList"} . ")" if($raw);
    print "backup$rawtext\n";
}
elsif($ra->{"out"}{"serverList"} eq "")
{
    print "no_connection\n";
}
else
{
    print "unknown_server=" . $ra->{"out"}{"serverList"} . "\n";
}
