#!/bin/perl
#================================================================================
#
#    csmi/include/function_parse.pl
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

my $file_name  = $ARGV[0];
my $module     = $ARGV[1];

open( my $file, "<", $file_name ) or die "Can't open $file_name : $!";

my $funct_str = "";
while ( <$file> ) {
    if ( /^int/../\);$/ ) {
        $_ =~ s/[\n ]+/ /g;
        $funct_str .=  $_;

        if ($funct_str =~ /;/) {
            parse_funct($funct_str, $module);
            $funct_str = "";
        }
    }
}


close $file;

sub parse_funct{
    my $string = shift;
    my $module = shift;
    my @funct  = $string =~ /int[ ]*([^\(]*)\(([^)]*)\);/gx;

    my @parameters    = split /,/,$funct[1];
    my @python_params = ( );
    my @python_types  = ( );

    for ( @parameters ) {
        my $def_str     = "";
        my $closure_str = "";

        # Count the pointers and build the pointer string.
        my $ptr_count = ($_ =~ tr/\*//);
        
        # Extract the type
        my $type ="";
        if (  m/[ ]*([\w_]+)[\* ]*([\w_]+).*/  ) {
            push @python_params, $2;
            my $type = $1;
            if ( $type =~ /char/ ) { $ptr_count--; $type="char_p"}

            foreach my $i (1..$ptr_count) {
                $def_str .="ctypes.POINTER(";
                $closure_str .=")";
            }
            $def_str .= "c_$type$closure_str";
        }
        
        
        #push @python_params $def_str;
        push @python_types, $def_str;
    }

    my $argtypes     = join(", ", @python_types);
    my $funct_params = join(", ", @python_params);

    print "\n_${module}.$funct[0].argtypes($argtypes)\n\n";
    print "def $funct[0]($funct_params):\n\tglobal _${module}\n\tresult = _${module}.$funct[0]()\n\treturn int(result)\n";
}
