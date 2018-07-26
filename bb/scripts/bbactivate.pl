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

sub isRoot
{
    return 0 if($> > 0);
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
        syslog($level, $line);
    }
}

sub cmd
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

sub cat
{
    my($filename) = @_;
    output("Reading file '$filename'");
    open(TMP, $filename);
    my @lines = <TMP>;
    close(TMP);
    return join("", @lines);
}

BEGIN
{
    ($dir,$fn) = $0 =~ /(\S+)\/(\S+)/;
    $SCRIPTPATH=abs_path($dir);
    unshift(@INC, $SCRIPTPATH);
}

if(! isRoot())
{
    croak "$0 must be run under root authority";
}
setDefaults();
GetOptions(
    "nodelist=s"            => \$CFG{"nodelist"},
    "esslist=s"             => \$CFG{"esslist"},
    "configtempl=s"         => \$CFG{"configtempl"},
    "outputconfig=s"        => \$CFG{"outputconfig"},
    "offload!"              => \$CFG{"USE_NVMF_OFFLOAD"},
    "csm!"                  => \$CFG{"USE_CSM"},
    "server!"               => \$CFG{"bbServer"},
    "metadata=s"            => \$CFG{"metadata"}
    );

getNodeName();
if($CFG{"bbServer"})
{
    makeServerConfigFile();
    filterLVM();
    startServer();
}
else
{
    makeProxyConfigFile();
    configureNVMeTarget();
    configureVolumeGroup();
    startProxy();
}
exit(0);



sub setDefaults
{
    $CFG{"nodelist"} = "/etc/ibm/nodelist";
    $CFG{"esslist"}  = "/etc/ibm/esslist";
    $CFG{"configtempl"} = "/opt/ibm/bb/scripts/bb.cfg";
    $CFG{"outputconfig"} = "/etc/ibm/bb.cfg";
    $CFG{"USE_NVMF_OFFLOAD"} = 0;
    $CFG{"USE_CSM"} = 1;
    $CFG{"bbServer"} = 0;
    $CFG{"metadata"} = "";
}

sub getNodeName
{
    setprefix("getNodeName: ");
    $xcatinfo = "/opt/xcat/xcatinfo";
    if(! -f $xcatinfo)
    {
	output("Node was not deployed by xCAT, using hostname", LOG_ERR);
	$nodename = cmd("hostname");
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

sub makeServerConfigFile
{
    setprefix("makeServerConfigFile: ");
    requireFile($CFG{"configtempl"});
    
    my $bbcfgtemplate = cat($CFG{"configtempl"});
    my $json = decode_json($bbcfgtemplate);
    
    if($CFG{"metadata"} =~ /\S/)
    {
	$json->{"bb"}{"bbserverMetadataPath"} = $CFG{"metadata"};
    }
    
    my $jsonoo = JSON->new->allow_nonref;
    my $out = $jsonoo->pretty->encode( $json );
    open(TMP, ">$CFG{outputconfig}");
    print TMP $out;
    close(TMP);
}

sub makeProxyConfigFile
{
    setprefix("makeProxyConfigFile: ");
    requireFile($CFG{"nodelist"});
    requireFile($CFG{"esslist"});
    requireFile($CFG{"configtempl"});
    
    my $bbcfgtemplate = cat($CFG{"configtempl"});
    my $json = decode_json($bbcfgtemplate);
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
	@backup = split(/\s+/, $ess);
	@backupnames = split(/\s+/, $ess);
	push(@ESS, @backup);
	
	foreach $name (@backupnames)
	{
	    $name =~ s/\./_/g;
	    $name = "server$name";
	}
	push(@ESSNAME, @backupnames);
	
	if($#backup+1 > 1)
	{
	    for($x=0; $x<$#backup+1; $x++)
	    {
		$BACKUP{$ESS[$x]} = $#backup-$x;
	    }
	}
    }
    close(TMP);
    
    if(!exists $NODES{$nodename})
    {
	output("This node '$nodename' does not appear in node list", LOG_ERR);
	exit(3);
    }
    
    $numess = $#ESS+1;
    $compute_per_ess = ceil($numnodes / $numess);
    $index = int(floor($NODES{$nodename} / $compute_per_ess));
    $namespace = ($NODES{$nodename} % 8192) + 10;
    $primaryServer = $index;
    output("Number compute nodes: $numnodes");
    output("Number ESS nodes: $numess");
    output("Compute per ESS: $compute_per_ess");
    output("Namespace: $namespace");
    
    for($x=0; $x<$numess; $x++)
    {
	$tmp = $json->{"bb"}{"server0"};
	foreach $key (keys %{$tmp})
	{
	    $json->{"bb"}{$ESSNAME[$x]}{$key} = $tmp->{$key};
	}
	$json->{"bb"}{$ESSNAME[$x]}{"ssladdress"} = $ESS[$x];
    }

    $json->{"bb"}{"proxy"}{"servercfg"} = "bb." . $ESSNAME[$primaryServer];
    output("ESS:    $ESS[$primaryServer] (bb.proxy.servercfg=bb.$ESSNAME[$primaryServer])");
    
    if(exists $BACKUP{$ESS[$index]})
    {
	$backupServer  = $BACKUP{$ESS[$index]};
	$json->{"bb"}{"proxy"}{"backupcfg"} = "bb." . $ESSNAME[$backupServer];
	output("Backup: $ESS[$backupServer] (bb.proxy.backupcfg=bb.$ESSNAME[$backupServer])");
    }
    
    if($CFG{"USE_CSM"})
    {
	$json->{"bb"}{"proxy"}{"controller"} = "csm";
    }
    $json->{"bb"}{"cmd"}{"controller"} = "none";   # disable on compute nodes
    
    $cfgfile = $json;  # make global
    my $jsonoo = JSON->new->allow_nonref;
    my $out = $jsonoo->pretty->encode( $json );
    open(TMP, ">$CFG{outputconfig}");
    print TMP $out;
    close(TMP);
}

sub configureNVMeTarget
{
    setprefix("configuring NVMf: ");
    cmd("modprobe nvmet");
    cmd("modprobe nvmet-rdma");
    
    my $mtab = cat("/etc/mtab");
    if($mtab !~ /configfs/)
    {
	cmd("mount -t configfs none /sys/kernel/config");
	$mtab = cat("/etc/mtab");
    }
    ($configfs) = $mtab =~ /configfs\s+(\S+)/;
    output("Configfs found at: $configfs");
    
    my $nvmetjson = cat("$SCRIPTPATH/nvmet.json");
    my $json = decode_json($nvmetjson);
    my $ns  = $json->{"subsystems"}[0]{"namespaces"}[0]{"nsid"} = $namespace;
    my $nqn = $json->{"subsystems"}[0]{"nqn"};
    
    my $enabled = cat("$configfs/nvmet/subsystems/$nqn/namespaces/$ns/enable");
    if($enabled =~ /1/)
    {
	output("NVMe over Fabrics target has already been configured");
	return;
    }
    
    output("ipaddr: " . $json->{"ports"}[0]{"addr"}{"traddr"});
    
    my $ipaddr = cmd("ip addr show dev ib0 | grep \"inet \"");
    ($myip) = $ipaddr =~  /inet\s+(\S+?)\//;
    
    output("myip: $myip");
    
    if(! isNVMeTargetOffloadCapable())
    {
	output("Node is not capable of NVMe over Fabrics target offload");
	$CFG{"USE_NVMF_OFFLOAD"} = 0
    }
    my $state = "disabled";
    $state = "enabled" if($CFG{"USE_NVMF_OFFLOAD"});
    output("NVMe over Fabrics target offload is $state");
    
    $json->{"ports"}[0]{"addr"}{"traddr"} = $myip;
    $json->{"subsystems"}[0]{"offload"} = $CFG{"USE_NVMF_OFFLOAD"};
    $json->{"subsystems"}[0]{"namespaces"}[0]{"enable"} = !$CFG{"USE_NVMF_OFFLOAD"};  # workaround
    
    my $out = encode_json($json);    
    open(TMP, ">/etc/ibm/nvmet.json");
    print TMP $out;
    close(TMP);
    
    cmd("nvmetcli restore /etc/ibm/nvmet.json");
    
    if($CFG{"USE_NVMF_OFFLOAD"})  # workaround
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
    my $vgdata = cmd("vgdisplay $bbvgname", 1);
    if($vgdata !~ /VG Name/)
    {
        cmd("vgcreate -y $bbvgname /dev/nvme0n1");
    }
    
    setprefix("Removing stale LVs: ");
    my $lvdata = cmd("lvs --reportformat json $bbvgname");
    my $json = decode_json($lvdata);
    
    foreach $rep (@{ $json->{"report"} })
    {
        foreach $lv (@{ $rep->{"lv"} })
        {
            my $lvname = $lv->{"lv_name"};
            my $vgname = $lv->{"vg_name"};
            if($vgname eq $bbvgname)
            {
                my $ismounted = cmd("grep '/dev/mapper/$vgname-$lvname ' /proc/mounts", 1);
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
    my $nvmelistout = cmd("nvme list");      # "nvme list -o json" doesn't work well

    # Scan for "real" nvme devices, ignore NVMe over Fabrics connections that may have duplicate volume groups
    my $adddevices = "";
    foreach $line (split("\n", $nvmelistout))
    {
        my ($dev, $remainder) = $line =~ /(\S+)\s+(.*)/;
        $adddevices .= "\"a|$dev|\", "         if(($dev =~ /\/dev/)&&($remainder !~ /Linux/));
    }

    # If admin has already modified the global_filter from default, don't undo their changes.
    my $lvmetc = cat("/etc/lvm/lvm.conf");
    my $search  = '# global_filter = \[ \"a\|.*\/\|\" \]';  # RHEL7 default
    my $replace = 'global_filter = [ ' . $adddevices . '"r|/dev/nvme*n*|" ]';
    $lvmetc =~ s/$search/$replace/oe;
    open(TMP, ">/etc/lvm/lvm.conf");
    print TMP $lvmetc;
    close(TMP);

    # Tell LVM to redo its volume cache incase its tainted.
    cmd("vgscan --cache");
}
