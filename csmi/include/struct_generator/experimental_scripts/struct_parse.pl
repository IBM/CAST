#!/bin/perl
#================================================================================
#
#    csmi/include/struct_parse.pl
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

my $file_name   = $ARGV[0];
my $struct_name = $ARGV[1];

open( my $file, "<", $file_name ) or die "Can't open $file_name : $!";

my @fields = ();

my $struct_str = "";
while ( <$file> ) {
    if ( /^CSMI_STRUCT/../\)$/ ) {
        $_ =~ s/[\n ]+/ /g;
        $struct_str .=  $_;
        
        if ( $struct_str =~ /\)/ ){
           # print "STRUCT $struct_str\n";
            my $s_type = parse_field($struct_str);
            if ( defined $s_type )
            {
                push @fields, $s_type;
            }
            $struct_str = "";
        }
    }
}

my $fields_str = join(",\n\t", @fields);
print "class c_${struct_name}(ctypes.Structure):\n\t_fields_=[$fields_str]\n\n\n";

close $file;

sub parse_field {
    my $type = "";
    my $field = shift;
    
    # type name csm_type length
    if ( m/[\w_]*\([ ]*([^ ,]*)[ ,]*([^ ,]*)[ ,]*([^ ,]*)[ ,]*.*/ ){
        my ($type, $name, $csm_type, $length) = ($1, $2, $3, $4);
        my $ptr_count = ( $type =~ tr/\*//);
        if ( $type =~ /char/ ) { $ptr_count--}

        
        my $def_str     = "";
        my $closure_str = "";

        foreach my $i (1..$ptr_count) {
            $def_str .="ctypes.POINTER(";
            $closure_str .=")";
        }
        
        # catches enumerated types.
        if ( $csm_type =~ "BASIC" and $type =~ /csm/ )
        {
            $type = "uint32_t";
        }

        if ( defined $csm_type and $csm_type =~ "FIXED" ) {
            return undef;
        }
        else
        {
            $type =~ s/\*//g;
            $type = "${def_str}ctypes.c_$type${closure_str}";
        }

        return "('$name', $type)";
    }

    return undef;
} 
