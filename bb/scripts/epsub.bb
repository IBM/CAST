#!/usr/bin/perl
###########################################################
#     epsub.bb
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

#$bbtmpdir = "/tmp";
@pwentry   = getpwnam($ENV{"USER"});
@pwentry   = getpwnam($ENV{"LSF_STAGE_USER"}) if(exists $ENV{"LSF_STAGE_USER"});
$bbtmpdir  = $pwentry[7] . "/.bbtmp";
$bbenvfile = $bbtmpdir . "/env." . $ENV{LSB_SUB_JOB_ID};

sub bbfail
{
    my($desc) = @_;
    print "$desc\n";
    die $desc;
}

sub openBBENV
{
    system("echo $bbenvfile > /tmp/epsubfile");
    open(BBENV, ">". $bbenvfile) || bbfail "Unable to open BB_ENVFILE file.  $!";
    
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
	$atime,$mtime,$ctime,$blksize,$blocks)= stat(BBENV) || bbfail "Unable to stat $bbenvfile.  $!";
    if($mode & 0077)
    {
        bbfail "Permissions on $bbenvfile are too broad, the group and world fields should be zero.";
    }
}


if(!-d $bbtmpdir)
{
    mkdir($bbtmpdir, 0700);
}

open(TMP, $ENV{LSB_SUB_PARM_FILE});
while($line = <TMP>)
{
    chomp($line);
    ($var,$value) = $line =~ /(\S+?)=(\S+)/;
    $ENV{$var} = $value;
}
close(TMP);

@vars = ("all");
if(exists $ENV{LSF_SUB4_SUB_ENV_VARS})
{
    @vars = split(",", $ENV{LSF_SUB4_SUB_ENV_VARS});
    push(@vars, "BBPATH="         . $ENV{"BBPATH"})         if(exists $ENV{"BBPATH"});
    push(@vars, "BSCFS_MNT_PATH=" . $ENV{"BSCFS_MNT_PATH"}) if(exists $ENV{"BSCFS_MNT_PATH"});;
}

openBBENV();
foreach $var (@vars)
{
    if($var =~ /all/)
    {
	foreach $key (keys %ENV)
	{
	    print BBENV "$key=$ENV{$key}\n" if(exists $ENV{$key});
	}
    }
    else
    {
	($key,$value) = $var =~ /(\S+?)=(\S+)/;
	print BBENV "$key=$value\n";
    }
}
close(BBENV);
