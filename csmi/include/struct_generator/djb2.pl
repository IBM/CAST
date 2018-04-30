#!/bin/perl 
#================================================================================
#
#    csmi/include/generate_struct.pl
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

use strict;
use warnings;
my $string = $ARGV[0];

# DJB2 hashing has counterpart in C code. 32 bit
my $hash = 5381;

foreach my $char (split //, $string) {
    $hash = ((( $hash << 5) + $hash ) + ord($char)) & 0xFFFFFFFF;
}
printf("0x%x\n", $hash);
