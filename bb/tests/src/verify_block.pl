#!/usr/bin/perl
###########################################################
#     verify_block.pl
#
#     Copyright IBM Corporation 2017. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

use POSIX;

($mountpt, $expected_size, $scribblesize) = @ARGV;

$d = `lsblk -o SIZE,MOUNTPOINT | grep $mountpt`;
chomp($d);
print "$d\n";
exit(-1) if(fabs(convertToBytes($expected_size) - convertToBytes($d)) > 2.0**30);

$v = convertToBytes($scribblesize);
cmd("dd if=/dev/urandom of=$mountpt/masterfile bs=1 count=$v");
cmd("split -b 500K -a 199 $mountpt/masterfile $mountpt/FVTST");
cmd("rm -rf ${mountpoint}/masterfile ${mountpoint}/FVTST*");
exit(0);

sub cmd
{
    my($cmd, $errtext) = @_;
    print STDERR "cmd: $cmd\n";
    system($cmd);
    exit(-1) if($? != 0);
}
sub convertToBytes
{
    my($val) = @_;
    $rval = 0;
    $rval = floor($val * 2.0**40) if($val =~ /T/);
    $rval = floor($val * 2.0**30) if($val =~ /G/);
    $rval = floor($val * 2.0**20) if($val =~ /M/);
    $rval = floor($val * 2.0**10) if($val =~ /K/);
    print "converted: $val to $rval\n";
    return $rval;
}
