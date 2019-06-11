#!/usr/bin/perl
###########################################################
#     bbtools.pm
#
#     Copyright IBM Corporation 2017,2018. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

package bbtools;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);

require Exporter;
require AutoLoader;
@ISA = qw(Exporter AutoLoader);
@EXPORT = qw(
             bbcmd
             bbgetrc
             bbgetsuccess
             bpost
             cmd
             setupUserEnvironment
             bbfail
             bbwaitTransfersComplete
);

$VERSION = '0.01';

use JSON;
use Sys::Hostname;
use File::Temp qw/ tempfile /;

sub isSetuid
{
    return 0 if($> > 0);
    return 0 if($< == 0);
    return 1;
}

$FLOOR      = "/opt/ibm/";
$FLOOR      = $ENV{"FLOOR"} if((! isSetuid()) && (exists $ENV{"FLOOR"}));
$BBCMDPATH  = "$FLOOR/bb/bin";
$QUIET      = $ENV{"BBTOOLS_QUIET"} if(! isSetuid());

$::SRMTYPE = "shell";
$::thishost = hostname();

@::HOSTLIST_ARRAY = ();
if((exists $ENV{"LSF_STAGE_HOSTFILE"}) && (-r $ENV{"LSF_STAGE_HOSTFILE"}))
{
    $::SRMTYPE = "LSF";
    open(TMP, $ENV{"LSF_STAGE_HOSTFILE"});
    while($n = <TMP>)
    {
        chomp($n);
        push(@::HOSTLIST_ARRAY, $n);
    }
    close(TMP);
    shift @::HOSTLIST_ARRAY;
}
elsif(exists $ENV{"LSF_STAGE_HOSTS"})
{
    $::SRMTYPE = "LSF";
    @::HOSTLIST_ARRAY = split(/\s+/, $ENV{"LSF_STAGE_HOSTS"});
    shift @::HOSTLIST_ARRAY;
}
elsif((exists $ENV{"LSB_DJOB_HOSTFILE"}) && (-r $ENV{"LSB_DJOB_HOSTFILE"}))
{
    $::SRMTYPE = "LSF";
    open(TMP, $ENV{"LSB_DJOB_HOSTFILE"});
    while($n = <TMP>)
    {
        chomp($n);
        push(@::HOSTLIST_ARRAY, $n);
    }
    close(TMP);
    if($::HOSTLIST_ARRAY[0] ne $::thishost)
    {
        print "Launch node hostname is $::HOSTLIST_ARRAY[0], but this node is $::thishost.  Bailing\n";
        exit(0);
    }
    shift @::HOSTLIST_ARRAY;
}
elsif(exists $ENV{"LSB_MCPU_HOSTS"})
{
    $::SRMTYPE = "LSF";
    my @MHOSTS = split(/\s+/, $ENV{"LSB_MCPU_HOSTS"});
    for($i = 0 ; $i < $#MHOSTS + 1 ; $i = $i + 2)
    {
        for($j = 0 ; $j < $MHOSTS[ $i + 1 ] ; $j++)
        {
            push(@::HOSTLIST_ARRAY, $MHOSTS[$i]);
        }
    }
    if($::HOSTLIST_ARRAY[0] ne $::thishost)
    {
        print "Launch node hostname is $::HOSTLIST_ARRAY[0], but this node is $::thishost.  Bailing\n";
        exit(0);
    }
    shift @::HOSTLIST_ARRAY;
}
elsif(exists $ENV{"LSB_HOSTS"})
{
    $::SRMTYPE = "LSF";
    @::HOSTLIST_ARRAY = split(/\s+/, $ENV{"LSB_HOSTS"});
    if($::HOSTLIST_ARRAY[0] ne $::thishost)
    {
        print "Launch node hostname is $::HOSTLIST_ARRAY[0], but this node is $::thishost.  Bailing\n";
        exit(0);
    }
    shift @::HOSTLIST_ARRAY;
}
else
{
    $::SRMTYPE = "shell";

    my $hostlist = $::DEFAULT_HOSTLIST;
    use Getopt::Long;

    $::JOBID = 0;

    GetOptions(
        "hosts=s" => \$hostlist,
        "jobid=i" => \$::JOBID,
        @::GETOPS
    ) or die("Invalid command line arguments.  Bailing\n");
    
    if($hostlist ne "")
    {
        @::HOSTLIST_ARRAY = split(/,/, $hostlist);
    }
    else
    {
        print "Valid HOSTLIST environment variable not defined.  Bailing\n";
        exit(0);
    }
}


if($::SRMTYPE eq "LSF")
{
    # From LSF environment variables
    $::BB_SSD_MIN        = $ENV{"LSF_STAGE_STORAGE_MINSIZE"};
    $::BB_SSD_MAX        = $ENV{"LSF_STAGE_STORAGE_MAXSIZE"};
    $::BB_STGIN_SCRIPT   = $ENV{"LSF_STAGE_USER_STAGE_IN"};
    
    my @stgout = split(",", $ENV{"LSF_STAGE_USER_STAGE_OUT"});
    $::BB_STGOUT1_SCRIPT = $stgout[0];
    $::BB_STGOUT2_SCRIPT = $stgout[1];
    
    $::JOBID             = $ENV{"LSF_STAGE_JOBID"};
    $::JOBID             = $ENV{"LSB_JOBID"} if(!exists $ENV{"LSF_STAGE_JOBID"});
    $::JOBUSER           = $ENV{"LSF_STAGE_USER"};
    $::BADEXITRC         = $ENV{"LSB_EXIT_PRE_ABORT"};
    $::BADNONRECOVEXITRC = $ENV{"LSB_EXIT_PRE_ABORT"};
    $::BADNONRECOVEXITRC = 125 if(exists $ENV{"LSF_STAGE_JOBID"});
    $::BPOSTMBOX         = 0;
    setupBBPATH();
}
elsif($::SRMTYPE eq "shell")
{
    # From user environment variables (for now)
    $::BB_SSD_MIN        = $ENV{"BB_SSD_MIN"};
    $::BB_SSD_MAX        = $ENV{"BB_SSD_MAX"};
    $::BB_STGIN_SCRIPT   = $ENV{"BB_STGIN_SCRIPT"};
    $::BB_STGOUT1_SCRIPT = $ENV{"BB_STGOUT1_SCRIPT"};
    $::BB_STGOUT2_SCRIPT = $ENV{"BB_STGOUT2_SCRIPT"};
    $::JOBID             = $::JOBID;  # filled in above
    $::JOBUSER           = $ENV{"USER"};
    $::BADEXITRC         = 1;
}


my %mkuniq   = map { $_, 1 } @::HOSTLIST_ARRAY;
@::HOSTLIST_ARRAY = keys %mkuniq;

$::HOSTLIST = join(",", @::HOSTLIST_ARRAY);
$::JOBUSER = untaint($::JOBUSER);
$oldpath = $ENV{'PATH'};
$ENV{'PATH'} = '/bin:/usr/bin';
chomp($::JOBGROUP=`/usr/bin/id -ng $::JOBUSER`);
$ENV{'PATH'} = $oldpath;

$::BBPATH="/tmp/bblv_$::JOBUSER\_$::JOBID";
$::BBPATH = $ENV{"BBPATH"} if($ENV{"BBPATH"} ne "");
$ENV{"BBPATH"} = $::BBPATH;
$::BBALL = join(",",0..$#::HOSTLIST_ARRAY);

eval
{
    $jsondata = `/bin/cat /etc/ibm/bb.cfg`;
    $::jsoncfg = $json = decode_json($jsondata);
    $controller = $json->{"bb"}{"cmd"}{"controller"};
};

$hl = "";
if($controller !~ /csm/i)
{
    $hl = " --hostlist=$::HOSTLIST";
}
$::TARGET_NODE0 = "--jobstepid=1$hl --target=0";
$::TARGET_ALL   = "--jobstepid=1$hl --target=0- --bcast";
$::TARGET_QUERY = "--jobstepid=0$hl --target=0";

if(exists $ENV{"BSCFS_MNT_PATH"})
{
    &setupBSCFS();
}

sub bbfail
{
    my($desc) = @_;
    bpost($desc);
    exit($::BADNONRECOVEXITRC);
}

sub untaint
{
    my($data) = @_;
    if($data =~ /^(.*)$/s)
    {
        return $1;
    }
    else
    {
        bbfail "Bad data in '$data'";
    }
}


sub bpost
{
    my($desc, $mailbox, $filedata) = @_;
    $bpostbin = $ENV{'LSF_BINDIR'};
    if(($bpostbin ne "") && ($::SRMTYPE eq "LSF"))    # LSF available
    {
        my $fh;
        my $tmpfilename;
        my $fileoption = "";
        $mailbox = $::BPOSTMBOX if(!defined $mailbox);
        if($filedata ne "")
        {
            ($fh, $tmpfilename) = tempfile();
            $fileoption = "-a $tmpfilename";
            print $fh $filedata;
        }
        cmd("$bpostbin/bpost $fileoption -d '$desc' -i $mailbox $::JOBID");
        
        if($filedata ne "")
        {
            close($fh);
            unlink($tmpfilename);
        }
    }
    else
    {
        print "Message: $desc\n";
    }
}

sub bbwaitTransfersComplete
{
    my $timeout = 3600;
    $timeout = $json->{"bb"}{"scripts"}{"transfertimeout"} if(exists $json->{"bb"}{"scripts"}{"transfertimeout"});

    my $starttime  = time();
    my $result     = bbcmd("$::TARGET_QUERY gettransfers --numhandles=0 --match=BBINPROGRESS");
    my $numpending = $result->{"0"}{"out"}{"numavailhandles"};
    while ($numpending > 0)
    {
        bpost("BB: Waiting for $numpending transfer(s) to complete");
        sleep(5);
        $result     = bbcmd("$::TARGET_QUERY gettransfers --numhandles=0 --match=BBINPROGRESS");
        $numpending = $result->{"0"}{"out"}{"numavailhandles"};
        my $curtime = time();
        last if($curtime - $starttime > $timeout);
        last if($result->{"rc"});
    }
    return $result;
}

sub bbcmd
{
    my($foo) = @_;
    $cmd = untaint($foo);
    if(!$QUIET) { print "\ncmd: $BBCMDPATH/bbcmd $cmd\n" }

    my $timeout = 600;
    $timeout = $json->{"bb"}{"scripts"}{"bbcmdtimeout"} if(exists $json->{"bb"}{"scripts"}{"bbcmdtimeout"});

    alarm($timeout);
    eval 
    {
        $oldpath     = $ENV{'PATH'};
        $ENV{'PATH'} = '/bin:/usr/bin';
        $jsonoutput  = `$BBCMDPATH/bbcmd $cmd`;
        $jsonoutput  = untaint($jsonoutput);
        $ENV{'PATH'} = $oldpath;

        if(!$QUIET) { print "json: $jsonoutput\n" }

        $result = decode_json($jsonoutput);
        if(!$QUIET) { printf("rc = %s\n", $result->{"rc"}) }
        if($result->{"rc"})
        {
            if(!$QUIET) { printf("Command failure.  rc=%s\n", $result->{"rc"}); }
        }
    };
    alarm(0);
    if($@)
    {
        print "bbcmd() eval error: $@  jsonoutput=$jsonoutput\n";
        $result = decode_json('{ "rc":1 }');
        $result->{"evalerr"} = $@;
    }
    return $result;
}


sub bbgetrc
{
    my($json) = @_;
    return $json->{"rc"};
}

sub bbgetsuccess
{
    my($json) = @_;
    return $json->{"goodcount"};
}

sub killpids
{
    open(TMP, "ps -o pid,ppid | grep $$ |");
    while($line = <TMP>)
    {
        ($pid,$ppid) = $line =~ /(\d+)\s+(\d+)/;
        if($ppid == $$)
        {
            system("kill -9 $pid");
        }
    }
    close(TMP);
}

sub cmd
{
    my($cmd, $timeout, $postresults) = @_;
    undef $::LASTOUTPUT;
    $cmd = untaint($cmd);
    if(!$QUIET) 
    { 
        print "cmd: $cmd\n";
    }
    $oldpath = $ENV{'PATH'};
    $ENV{'PATH'} = '/bin:/usr/bin';

    eval
    {
        local $SIG{"ALRM"} = sub { die "alarm timeout\n" };
        alarm($timeout) if($timeout);
        if($postresults)
        {
            my @lines = ();
            open(CMD, "$cmd |") || die "Unable to open $cmd";
            local $SIG{"ALRM"} = sub { killpids(); close(CMD); die "alarm timeout\n" };
            while(my $line = <CMD>)
            {
                chomp($line);
                push(@lines, $line);
            }
            close(CMD);
            $::LASTOUTPUT = join("\n", @lines);
        }
        else
        {
            system($cmd);
        }
        alarm(0);
    };
    alarm(0);

    $rc = $?;
    $rc = 110 if($@ =~ /alarm timeout/);  # ETIMEDOUT=110
    $rc = 2   if($@ =~ /Unable to open/); # ENOENT=2

    print "command rc: $rc\n";
    $ENV{'PATH'} = $oldpath;
    return $rc;
}


sub defaultenv
{
    my($var, $key, $value) = @_;
    if($ENV{$var} eq "")
    {
        $value = $json->{"bb"}{"bscfsAgent"}{$key} if(exists $json->{"bb"}{"bscfsAgent"}{$key});
        $ENV{$var} = $value;
    }
}
sub setupBSCFS
{
    if(!exists $ENV{BSCFS_WORK_PATH})
    {
        eval
        {
            $jsondata = `/bin/cat /etc/ibm/bb.cfg`;
            $::jsoncfg = $json = decode_json($jsondata);
        };
        $ENV{BSCFS_WORK_PATH} = $json->{"bb"}{"bscfsagent"}{"workpath"} . "/$::JOBID." . $ENV{"BBHASH"};
    }
    
    $::BSCFS_BB_PATH    = $ENV{BBPATH} . "/.bscfs";
    $::CLEANUP_LIST     = $ENV{BSCFS_WORK_PATH} . "/cleanup_list";
    $::PRE_INSTALL_LIST = $ENV{BSCFS_WORK_PATH} . "/pre_install_list";
    
    defaultenv("BSCFS_WRITE_BUFFER_SIZE", "write_buffer_size", 4194304);
    defaultenv("BSCFS_READ_BUFFER_SIZE", "read_buffer_size", 16777216);
    defaultenv("BSCFS_DATA_FALLOC_SIZE", "data_falloc_size", 0);
    defaultenv("BSCFS_MAX_INDEX_SIZE", "max_index_size", 4294967296);
    defaultenv("BSCFS_PFS_PATH", "pfs_path", $ENV{LS_EXECCWD});
    defaultenv("BSCFS_MNT_PATH", "local_path", "/bscfs");
}

sub openBBENV
{
    my $bbenvfile;
    my $envnotready = 60;
    ($BBENV, $bbenvfile) = tempfile(UNLINK => 1);
    $bpostbin = $ENV{'LSF_BINDIR'};
    do 
    {
        system("$bpostbin/bread -a $bbenvfile -i 119 $::JOBID");
        if(! -f $bbenvfile)
        {
            $envnotready--;
            sleep(1);
        }
        else
        {
            $envnotready = -1;
        }
    }
    while($envnotready > 0);
    return -1 if($envnotready == 0);
    return 0;
}

sub setupBBPATH
{
    my $bpostbin = $ENV{'LSF_BINDIR'};
    my $bbpathdata = `$bpostbin/bread -w -i 119 $::JOBID`;  # root calls this, cannot use files
    ($ENV{"BBPATH"} = $::BBPATH) = $bbpathdata =~ /BB Path=(\S+)/;
}

sub setupUserEnvironment
{
    $ENV{"PATH"} = $ENV{"PATH_PRESERVE"};
    my $rc = openBBENV();
    return -1 if($rc);
    while($line = <$BBENV>)
    {
        chomp($line);
        ($key, $value) = $line =~ /(\S+)=(.*)/;
        $ENV{$key} = $value;
    }
    close($BBENV);

    if(exists $ENV{"LSB_SUB3_CWD"})
    {
        my $dir = $ENV{"LSB_SUB3_CWD"};
        ($dir) =~ s/^\"(.*)\"/$1/;
        chdir($dir);
    }
    elsif(exists $ENV{"PWD"})
    {
        chdir($ENV{"PWD"});
    }
    return 0;
}

1;
