#!/usr/bin/perl
####################################################
#    genbbcfg.pl
#
#    Copyright IBM Corporation 2016,2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

($cfgin, $cfgout, $rpmbuild) = @ARGV;

$data = `cat $cfgin`;

$metadatapath  = "/gpfs/gpfs0/bbmetadata";
$sslport    = 9001;
$usersuffix = "";
$consoleenable = "false";

if(!$rpmbuild)
{
    my $user = "juser";
    $user = $ENV{"USER"} if(exists $ENV{"USER"});
    $metadatapath  = "/gpfs/gpfs0/$user/bbserver/bbmetadata";
    $userindex  = `grep -n $user /etc/passwd`;
    $userindex  =~ s/:.*//;
    $sslport    = 9000 + $userindex;
    $usersuffix = ".$user";
    $consoleenable = "true";
}

$data =~ s/\@METADATAPATH\@/$metadatapath/g;
$data =~ s/\@SSLPORT\@/$sslport/g;
$data =~ s/\@USERSUFFIX\@/$usersuffix/g;
$data =~ s/\@USECONSOLE\@/$consoleenable/g;

open(TMP, ">$cfgout");
print TMP $data;
close(TMP);
