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
    my($out) = @_;
    foreach $line (split("\n", $out))
    {
	print "$outputprefix$line\n";
    }
}

sub cmd
{
    my($cmd) = @_;
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
	die "Command '$cmd' failed.  Aborting $0";
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
    "csm!"                  => \$CFG{"USE_CSM"}
    );

getNodeName();
makeConfigFile();
configureNVMeTarget();
configureVolumeGroup();
startProxy();
exit(0);



sub setDefaults
{
    $CFG{"nodelist"} = "/etc/ibm/nodelist";
    $CFG{"esslist"}  = "/etc/ibm/esslist";
    $CFG{"configtempl"} = "/opt/ibm/bb/scripts/bb.cfg";
    $CFG{"outputconfig"} = "/etc/ibm/bb.cfg";
    $CFG{"USE_NVMF_OFFLOAD"} = 0;
    $CFG{"USE_CSM"} = 1;
}

sub getNodeName
{
    setprefix("getNodeName: ");
    $xcatinfo = "/opt/xcat/xcatinfo";
    if(! -f $xcatinfo)
    {
	croak "Node was not deployed by xCAT.  Unable to identify the correct hostname";
    }
    $data = cat($xcatinfo);
    ($nodename) = $data =~ /NODE=(\S+)/s;
    output("node: $nodename");
}

sub makeConfigFile
{
    setprefix("makeConfigFile: ");
    return if(!-f $CFG{"nodelist"});
    return if(!-f $CFG{"esslist"});
    return if(!-f $CFG{"configtempl"});
    $bbcfgtemplate = cat($CFG{"configtempl"});
    $json = decode_json($bbcfgtemplate);
    
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
    
    croak "This node '$nodename' does not appear in node list" if(!exists $NODES{$nodename});
    
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
    
    $cfgfile = $json;
    $out = encode_json($json);
    open(TMP, ">$CFG{outputconfig}");
    print TMP $out;
    close(TMP);
}

sub configureNVMeTarget
{
    setprefix("configuring NVMf: ");
    cmd("modprobe nvmet");
    cmd("modprobe nvmet-rdma");
    
    $nvmetjson = cat("$SCRIPTPATH/nvmet.json");
    $json = decode_json($nvmetjson);
    
    $ns  = $json->{"subsystems"}[0]{"namespaces"}[0]{"nsid"} = $namespace;
    $nqn = $json->{"subsystems"}[0]{"nqn"};
    
    $enabled = cat("/sys/kernel/config/nvmet/subsystems/$nqn/namespaces/$ns/enable");
    if($enabled =~ /1/)
    {
	output("NVMe over Fabrics target has been configured");
	return;
    }
    
    output("ipaddr: " . $json->{"ports"}[0]{"addr"}{"traddr"});
    
    $ipaddr = cmd("ip addr show dev ib0 | grep \"inet \"");
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
    
    $out = encode_json($json);    
    open(TMP, ">/etc/ibm/nvmet.json");
    print TMP $out;
    close(TMP);
    
    cmd("nvmetcli restore /etc/ibm/nvmet.json");
    
    if($CFG{"USE_NVMF_OFFLOAD"})  # workaround
    {
	cmd("rm -f /sys/kernel/config/nvmet/ports/1/subsystems/$nqn");
	cmd("echo 1 > /sys/kernel/config/nvmet/subsystems/$nqn/attr_offload");
	cmd("echo 1 > /sys/kernel/config/nvmet/subsystems/$nqn/namespaces/$ns/enable");
	cmd("ln -s /sys/kernel/config/nvmet/subsystems/$nqn /sys/kernel/config/nvmet/ports/1/subsystems/$nqn");
    }
}

sub configureVolumeGroup
{
    setprefix("Configuring VG: ");
    $bbvgname = $cfgfile->{"bb"}{"proxy"}{"volumegroup"};
    
    cmd("vgscan --cache");
    eval
    {
	$vgdata = cmd("vgdisplay $vgname");
    };
    if($vgdata eq "")
    {
	cmd("vgcreate -y $vgname /dev/nvme0n1");
    }
    
    setprefix("Removing stale LVs: ");
    $lvdata = cmd("lvs --reportformat json $vgname");
    $json = decode_json($lvdata);
    
    foreach $rep (@{ $json->{"report"} })
    {
	foreach $lv (@{ $rep->{"lv"} })
	{
	    $lvname   = $lv->{"lv_name"};
	    $vgname = $lv->{"vg_name"};
	    if($vgname eq $bbvgname)
	    {
		my $ismounted = "";
		eval
		{
		    $ismounted = cmd("grep '/dev/mapper/$vgname-$lvname ' /proc/mounts");
		};
		print "ismounted: $ismounted\n";
		if($ismounted !~ /\S/)
		{
		    cmd("lvremove -f /dev/$vgname/$lvname");
		}
	    }
	}
    }
}

sub startProxy
{
    setprefix("Starting bbProxy: ");
    cmd("service bbproxy start");
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
