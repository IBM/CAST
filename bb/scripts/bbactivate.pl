#!/usr/bin/perl
###########################################################
#     bbactivate.pl
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
use Carp qw( croak );
use Getopt::Long;
use POSIX;
use Sys::Syslog;
use File::Temp qw/ tempfile /;

BEGIN 
{ 
    my ($dir) = $0 =~ /(\S+)\//;
    unshift(@INC, $::SCRIPTPATH=abs_path($dir));
}

sub isRoot { return ($>==0); }

sub setprefix
{
    my($pf) = @_;
    $outputprefix = $pf;
}

sub output
{
    my($out, $level) = @_;
    
    if(($setupSyslog == 0) && ($CFG{"dryrun"} == 0))
    {
        openlog("bbactivate", "ndelay,pid", "local0");
        $setupSyslog = 1;
    }
    if(!defined $level)
    {
        $level = LOG_INFO;
    }
    
    foreach $line (split("\n", $out))
    {
        print "$outputprefix$line\n";
        syslog($level, $line) if($setupSyslog);
    }
}

sub safe_cmd
{
    my($cmd, $ignoreFailure) = @_;
    my $timeout = 60;

    output("Running command: $cmd");
    
    alarm($timeout);
    eval
    {
        $rc = `$cmd 2>&1`;
    };
    alarm(0);

    $rc .= "PROBLEM: command took longer than $timeout seconds" if($@ =~ /alarm timeout/);
    
    output("Command result:  rc=$?   Evaluate=$@");
    output("Command result:  $rc");
    
    if(($? != 0) || ($@ =~ /alarm timeout/i))
    {
        if($ignoreFailure)
        {
            output("Command '$cmd' had exit status $?");
        }
        else
        {
            output("Command '$cmd' failed.  Aborting $0", LOG_ERR);
            exit(4);
        }
    }
    return $rc;
}

sub cmd
{
    my($cmd, $ignoreFailure) = @_;
    if($CFG{"dryrun"})
    {
        output("Would-run command: $cmd");
        return 0;
    }
    return safe_cmd($cmd, $ignoreFailure);
}

sub cat
{
    my($filename) = @_;
    output("Reading file '$filename'");
    open(TMP, $filename);
    my @lines = <TMP>;
    close(TMP);
    return join("", @lines);
}

sub writeConfiguration
{
    my($filename, $data) = @_;

    my $origfilename = $filename;
    my $supl = "";
    if($CFG{"dryrun"})
    {
        my $dp = $CFG{"drypath"};
        if($dp ne "&STDOUT")
        {
            $filename =~ s/.*\///og;
            $dp .= "/$filename";
        }
        $filename = $dp;
        $supl = " (actual file would have been $origfilename)";
    }
    output("Writing file to $filename$supl");
    open(TMP, ">$filename");
    print TMP $data;
    close(TMP);
}

setDefaults();
GetOptions(
    "nodelist=s"      => \$CFG{"nodelist"},
    "esslist=s"       => \$CFG{"esslist"},
    "configtempl=s"   => \$CFG{"configtempl"},
    "nvmetempl=s"     => \$CFG{"nvmetempl"},
    "outputconfig=s"  => \$CFG{"outputconfig"},
    "offload!"        => \$CFG{"USE_NVMF_OFFLOAD"},
    "csm!"            => \$CFG{"USE_CSM"},
    "cn!"             => \$CFG{"bbProxy"},
    "server!"         => \$CFG{"bbServer"},
    "ln!"             => \$CFG{"bbcmd"},
    "health!"         => \$CFG{"bbhealth"},
    "envdir=s"        => \$CFG{"envdir"},
    "lsfdir=s"        => \$CFG{"lsfdir"},
    "bscfswork=s"     => \$CFG{"bscfswork"},
    "sslcert=s"       => \$CFG{"sslcert"},
    "sslpriv=s"       => \$CFG{"sslpriv"},
    "dryrun!"         => \$CFG{"dryrun"},
    "drypath=s"       => \$CFG{"drypath"},
    "scriptpath=s"    => \$SCRIPTPATH,
    "metadata=s"      => \$CFG{"metadata"},
    "skip=s"          => \$CFG{"skip"},
    "help!"           => \$CFG{"help"}
);
setDefaults();

if($CFG{"help"})
{
    system("man bbactivate");
    exit(0);
}

output("Running: $0 @ARGV");
if(! isRoot())
{
    output("$0 must be run under root authority.  Exiting");
    exit(99);
}

getNodeName();
makeConfigFile() if($CFG{"skip"} !~ /config/i);

if($CFG{"bbServer"})
{
    filterLVM()   if($CFG{"skip"} !~ /lvm/i);
    startServer() if($CFG{"skip"} !~ /start/i);
}
if($CFG{"bbcmd"})
{
    copyBBFilesToLSF() if($CFG{"skip"} !~ /lsf/i);
}
if($CFG{"bbProxy"})
{
    configureNVMeTarget()  if($CFG{"skip"} !~ /nvme/i);
    configureVolumeGroup() if($CFG{"skip"} !~ /lvm/i);
    startProxy()           if($CFG{"skip"} !~ /start/i);
    startHealth()          if($CFG{"skip"} !~ /health/i);
}
exit(0);

sub def
{
    my($var, $phase, $value) = @_;
    $value = "DEFAULT" if($phase > $currentphase);
    $value = $CFG{$var} if(($phase == $currentphase) && ($phase > 1) && ($CFG{$var} ne "DEFAULT"));
    $CFG{$var} = $value if($phase >= $currentphase);
}

sub setDefaults
{
    $currentphase++;
    &def("nodelist",         1, "/etc/ibm/nodelist");
    &def("esslist",          1, "/etc/ibm/esslist");
    &def("outputconfig",     1, "/etc/ibm/bb.cfg");
    &def("drypath",          1, "&STDOUT");
    &def("USE_NVMF_OFFLOAD", 1, 0);
    &def("USE_CSM",          1, 1);
    &def("bbProxy",          1, 0);
    &def("bbServer",         1, 0);
    &def("bbcmd",            1, 0);
    &def("health",           1, 1);
    &def("sslcert",          1, "default");
    &def("sslpriv",          1, "default");
    &def("metadata",         1, "");
    &def("bscfswork",        1, "");
    &def("skip",             1, "");
    &def("configtempl",      2, "$SCRIPTPATH/bb.cfg");
    &def("nvmetempl",        2, "$SCRIPTPATH/nvmet.json");

    $CFG{"bbProxy"}=1    if(($currentphase == 2) && ($CFG{"bbProxy"}==0) && ($CFG{"bbServer"}==0) && ($CFG{"bbcmd"}==0));
}

sub getNodeName
{
    setprefix("getNodeName: ");
    $xcatinfo = "/opt/xcat/xcatinfo";
    if(!-f $xcatinfo)
    {
        output("Node was not deployed by xCAT, using hostname", LOG_ERR);
        $nodename = safe_cmd("hostname");
        chomp($nodename);
    }
    else
    {
        $data = cat($xcatinfo);
        ($nodename) = $data =~ /NODE=(\S+)/s;
    }
    output("node: $nodename");
}


sub requireFile
{
    my($file) = @_;
    if(!-f $file)
    {
        output("Specified file does not exist '$file'", LOG_ERR);
        exit(2);
    }
}


sub copyBBFilesToLSF
{
    my $lsfdir;
    if($CFG{"lsfdir"} ne "")
    {
        $lsfdir = $CFG{"lsfdir"};
    }
    else
    {
        output("Option --lsfdir=\$LSF_SERVERDIR was not specified.  Skipping BB script copy");
        return 0;
    }
    requireFile("$SCRIPTPATH/esub.bscfs");  # this one is a byproduct of install vs. git checkout, make sure its there.

    output("Copying BB scripts from $SCRIPTPATH");
    cmd("cp $SCRIPTPATH/esub.bb $lsfdir/.");
    cmd("cp $SCRIPTPATH/epsub.bb $lsfdir/.");
    cmd("cp $SCRIPTPATH/esub.bscfs $lsfdir/.");
    cmd("cp $SCRIPTPATH/epsub.bscfs $lsfdir/.");
    cmd("cp $SCRIPTPATH/bb_pre_exec.sh $lsfdir/.");
    cmd("cp $SCRIPTPATH/bb_post_exec.sh $lsfdir/.");
}

sub makeConfigFile
{
    setprefix("makeConfigFile: ");
    requireFile($CFG{"configtempl"});

    my $bbcfgtemplate = cat($CFG{"configtempl"});
    local $json = decode_json($bbcfgtemplate);

    &makeServerConfigFile();
    &makeProxyConfigFile();
    &makeLNConfigFile();

    $cfgfile = $json;    # make global

    my $jsonoo = JSON->new->allow_nonref->canonical;
    my $out    = $jsonoo->pretty->encode($json);
    writeConfiguration($CFG{"outputconfig"}, $out);
}

sub makeLNConfigFile
{
    setprefix("makeLNConfigFile: ");
    if(($CFG{"USE_CSM"}) && (!$CFG{"bbProxy"}))
    {
        $json->{"bb"}{"cmd"}{"controller"} = "csm";
    }
    else
    {
        $json->{"bb"}{"cmd"}{"controller"} = "none";
    }
    if($CFG{"envdir"} eq "HOME")
    {
        $json->{"bb"}{"envdir"} = "";
    }
    else
    {
        $json->{"bb"}{"envdir"} = $CFG{"envdir"};
    }
    if($CFG{"bscfswork"})
    {
        $json->{"bb"}{"bscfsagent"}{"workpath"} = $CFG{"bscfswork"};
    }
}

sub makeServerConfigFile
{
    setprefix("makeServerConfigFile: ");
    if($CFG{"metadata"} =~ /\S/)
    {
        $json->{"bb"}{"bbserverMetadataPath"} = $CFG{"metadata"};
    }
}

sub makeProxyConfigFile
{
    setprefix("makeProxyConfigFile: ");

    if($CFG{"USE_CSM"})
    {
        $json->{"bb"}{"proxy"}{"controller"} = "csm";
    }
    $json->{"bb"}{"cmd"}{"controller"} = "none";    # disable on compute nodes

    $json->{"bb"}{"server0"}{"sslcertif"} = $CFG{"sslcert"} if($CFG{"sslcert"} ne "default");
    $json->{"bb"}{"server0"}{"sslpriv"}   = $CFG{"sslpriv"} if($CFG{"sslpriv"} ne "default");

    return if(!$CFG{"bbProxy"});

    requireFile($CFG{"nodelist"});
    requireFile($CFG{"esslist"});
    requireFile($CFG{"configtempl"});

    my $numnodes = 0;
    open(TMP, $CFG{"nodelist"});
    while($node = <TMP>)
    {
        next if($node =~ /^#/);
        next if($node !~ /\S/);
        chomp($node);
        $NODES{$node} = $numnodes++;
    }
    close(TMP);
    open(TMP, $CFG{"esslist"});
    while($ess = <TMP>)
    {
        next if($ess =~ /^#/);
        next if($ess !~ /\S/);
        chomp($ess);
        @backup      = split(/\s+/, $ess);
        @backupnames = split(/\s+/, $ess);
        push(@ESS, @backup);

        foreach $name (@backupnames)
        {
            $name =~ s/\./_/g;
            $name = "server$name";
        }
        push(@ESSNAME, @backupnames);

        if($#backup + 1 > 1)
        {
            for($x = 0 ; $x < $#backup + 1 ; $x++)
            {
                $BACKUP{ $ESS[$x] } = $#backup - $x;
            }
        }
    }
    close(TMP);

    if(!exists $NODES{$nodename})
    {
        output("This node '$nodename' does not appear in node list", LOG_ERR);
        exit(3);
    }

    $numess          = $#ESS + 1;
    $compute_per_ess = ceil($numnodes / $numess);
    $index           = int(floor($NODES{$nodename} / $compute_per_ess));
    $namespace       = ($NODES{$nodename} % 8192) + 10;
    $primaryServer   = $index;
    output("Number compute nodes: $numnodes");
    output("Number ESS nodes: $numess");
    output("Compute per ESS: $compute_per_ess");
    output("Namespace: $namespace");

    for($x = 0 ; $x < $numess ; $x++)
    {
        $tmp = $json->{"bb"}{"server0"};
        foreach $key (keys %{$tmp})
        {
            $json->{"bb"}{ $ESSNAME[$x] }{$key} = $tmp->{$key};
        }
        $json->{"bb"}{ $ESSNAME[$x] }{"ssladdress"} = $ESS[$x];
    }

    $json->{"bb"}{"proxy"}{"servercfg"} = "bb." . $ESSNAME[$primaryServer];
    output("ESS:    $ESS[$primaryServer] (bb.proxy.servercfg=bb.$ESSNAME[$primaryServer])");

    if(exists $BACKUP{ $ESS[$index] })
    {
        $backupServer = $BACKUP{ $ESS[$index] };
        $json->{"bb"}{"proxy"}{"backupcfg"} = "bb." . $ESSNAME[$backupServer];
        output("Backup: $ESS[$backupServer] (bb.proxy.backupcfg=bb.$ESSNAME[$backupServer])");
    }

    delete $json->{"bb"}{"server0"} if(!$CFG{"bbServer"});
}

sub configureNVMeTarget
{
    setprefix("configuring NVMf: ");
    cmd("modprobe nvmet");
    cmd("modprobe nvmet-rdma");

    # Wait for kernel module(s) to complete load.  nvme driver may not have been loaded.
    my $timeout = 10;
    while(($timeout > 0) && (safe_cmd("lsmod | grep nvmet_rdma") !~ /nvmet_rdma/))
    {
        $timeout -= 1;
        sleep(1);
    }
    if($timeout == 0)
    {
        output("nvmet_rdma kernel module could not be loaded");
        exit(5);
    }

    my $mtab = cat("/etc/mtab");
    if($mtab !~ /configfs/)
    {
        cmd("mount -t configfs none /sys/kernel/config");
        $mtab = cat("/etc/mtab");
    }
    ($configfs) = $mtab =~ /configfs\s+(\S+)/;
    output("Configfs found at: $configfs");

    my $nvmetjson = cat($CFG{"nvmetempl"});
    my $json      = decode_json($nvmetjson);
    my $ns        = $json->{"subsystems"}[0]{"namespaces"}[0]{"nsid"} = $namespace;
    my $nqn       = $json->{"subsystems"}[0]{"nqn"};

    my $enabled = cat("$configfs/nvmet/subsystems/$nqn/namespaces/$ns/enable");
    if($enabled =~ /1/)
    {
        output("NVMe erer Fabrics target has already been configured");
        return;
    }

    output("ipaddr: " . $json->{"ports"}[0]{"addr"}{"traddr"});

    my $ipaddr = safe_cmd("ip addr show dev ib0 | grep \"inet \"");
    ($myip) = $ipaddr =~ /inet\s+(\S+?)\//;

    output("myip: $myip");

    if(!isNVMeTargetOffloadCapable())
    {
        output("Node is not capable of NVMe over Fabrics target offload");
        $CFG{"USE_NVMF_OFFLOAD"} = 0;
    }
    my $state = "disabled";
    $state = "enabled" if($CFG{"USE_NVMF_OFFLOAD"});
    output("NVMe over Fabrics target offload is $state");

    $json->{"ports"}[0]{"addr"}{"traddr"}               = $myip;
    $json->{"subsystems"}[0]{"offload"}                 = $CFG{"USE_NVMF_OFFLOAD"};
    $json->{"subsystems"}[0]{"namespaces"}[0]{"enable"} = !$CFG{"USE_NVMF_OFFLOAD"};    # workaround

    my $jsonoo = JSON->new->allow_nonref->canonical;
    my $out    = $jsonoo->pretty->encode($json);

    my $cleanup = 1;
    $cleanup    = 0 if($CFG{"skip"} =~ /cleanup/);
    my ($fh, $tmpfilename) = tempfile(SUFFIX => ".nvmet.json", UNLINK => $cleanup);
    writeConfiguration($tmpfilename, $out);
    cmd("nvmetcli restore " . $tmpfilename);

    if($CFG{"USE_NVMF_OFFLOAD"})                                                        # workaround
    {
        cmd("rm -f $configfs/nvmet/ports/1/subsystems/$nqn");
        cmd("echo 1 > $configfs/nvmet/subsystems/$nqn/attr_offload");
        cmd("echo 1 > $configfs/nvmet/subsystems/$nqn/namespaces/$ns/enable");
        cmd("ln -s $configfs/nvmet/subsystems/$nqn $configfs/nvmet/ports/1/subsystems/$nqn");
    }
}


sub configureVolumeGroup
{
    setprefix("Configuring VG: ");
    my $bbvgname = $cfgfile->{"bb"}{"proxy"}{"volumegroup"};
    
    cmd("vgscan --cache");
    my $vgdata = safe_cmd("vgdisplay $bbvgname", 1);
    if($vgdata !~ /VG Name/)
    {
        cmd("vgcreate -y $bbvgname /dev/nvme0n1");
    }
    
    setprefix("Removing stale LVs: ");
    my $lvdata = safe_cmd("lvs --reportformat json $bbvgname");
    my $json = decode_json($lvdata);
    
    foreach $rep (@{ $json->{"report"} })
    {
        foreach $lv (@{ $rep->{"lv"} })
        {
            my $lvname = $lv->{"lv_name"};
            my $vgname = $lv->{"vg_name"};
            if($vgname eq $bbvgname)
            {
                my $dmpath = "/dev/mapper/$vgname-$lvname";
                my $swappath = abs_path($dmpath);
                my $isswap = safe_cmd("grep '$swappath ' /proc/swaps", 1);
                if($isswap =~ /\S/)
                {
                    output("Volume group $vgname, logical volume $lvname ($dmpath -> $swappath) was referenced in /proc/swaps.  Skipping");
                    next;
                }
                my $ismounted = safe_cmd("grep '$dmpath ' /proc/mounts", 1);
                output("Mounted $vgname-$lvname at: $ismounted");
                if($ismounted !~ /\S/)
                {
                    cmd("lvremove -f /dev/$vgname/$lvname");
                }
            }
        }
    }
}

sub startServer
{
    setprefix("Starting bbServer: ");
    cmd("service bbserver restart");
}

sub startProxy
{
    setprefix("Starting bbProxy: ");
    cmd("service bbproxy restart");
}

sub startHealth
{
    setprefix("Starting bbHealth: ");
    cmd("service bbhealth restart") if($CFG{"health"});
}

sub isNVMeTargetOffloadCapable
{
    my $p2p = cat("/sys/block/nvme0n1/device/num_p2p_queues");
    return 0 if($p2p =~ /0/);
    
    my $proc = cat("/proc/cpuinfo");
    my $p2p_processor_found = 0;
    $p2p_processor_found = 1 if($proc =~ /POWER9/);
    
    return $p2p_processor_found;
}

sub filterLVM
{
    my $nvmelistout = safe_cmd("nvme list");      # "nvme list -o json" doesn't work well

    # Scan for "real" nvme devices, ignore NVMe over Fabrics connections that may have duplicate volume groups
    my $adddevices = "";
    foreach $line (split("\n", $nvmelistout))
    {
        my ($dev, $remainder) = $line =~ /(\S+)\s+(.*)/;
        $adddevices .= "\"a|$dev|\", "         if(($dev =~ /\/dev/)&&($remainder !~ /Linux/));
    }

    # If admin has already modified the global_filter from default, don't undo their changes.
    # todo:  consider if this would be better as a patch file rather than replace.
    my $lvmetc = cat("/etc/lvm/lvm.conf");
    my $search  = '# global_filter = \[ \"a\|.*\/\|\" \]';  # RHEL7 default
    my $replace = 'global_filter = [ ' . $adddevices . '"r|/dev/nvme*n*|" ]';
    $lvmetc =~ s/$search/$replace/oe;
    
    writeConfiguration("/etc/lvm/lvm.conf", $lvmetc);

    # Tell LVM to redo its volume cache incase its tainted.
    cmd("vgscan --cache");
}
