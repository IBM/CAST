#!/usr/bin/perl
####################################################
#    gendefaults.pl
#
#    Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

($cfgin, $cfgout, $workdir, $rpmbuild) = @ARGV;

$data = `cat $cfgin`;

$cfgfile       = "/etc/ibm/bb.cfg";
$varpath       = "/tmp/bbproxy";
$metadatapath  = "/gpfs/gpfs0/bbmetadata";

if(!$rpmbuild)
{
    my $user = "juser";
    $user = $ENV{"USER"} if(exists $ENV{"USER"});
    $cfgfile       = "$workdir/bb/scripts/bb.cfg";
    $varpath       = "/tmp/bbproxy.$user";
    $metadatapath  = "/gpfs/gpfs0/$user/bbserver/bbmetadata";
}

$data =~ s/\@CONFIGFILE\@/$cfgfile/g;
$data =~ s/\@VARPATH\@/$varpath/g;
$data =~ s/\@METADATAPATH\@/$metadatapath/g;

$oldcfg = `cat $cfgout`;
if($oldcfg ne $data)
{
    open(TMP, ">$cfgout");
    print TMP $data;
    close(TMP);
}
