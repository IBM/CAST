#!/bin/perl 
#================================================================================
#
#    csmi/include/generate_print_formatter.pl
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
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

# The type resolver map, used to generate the formatter array and compute the resolver type.
my %type_resolver = (
    misc     => [0 ,"\"\""  ],
    string   => [1 ,"\"\""  ], # char*
    enum     => [2 ,"\"\""  ],
    enum_bit => [3 ,"\"\""  ], # TODO Implement.
    csm_bool => [4 , "\"\"" ], # Bool is a special case.
    uint8_t  => [5  ,"\"%\" PRIu8 "], # Do not place any printable types above this.
    uint32_t => [6 ,"\"%\" PRIu32"],
    uint64_t => [7 ,"\"%\" PRIu64"],
    int8_t   => [8 ,"\"%\" PRId8 "],
    int32_t  => [9 ,"\"%\" PRId32"],
    int64_t  => [10,"\"%\" PRId64"],
    int      => [11,"\"%d\"" ],
    short    => [12,"\"%d\"" ],
    long     => [13,"\"%ld\""],
    double   => [14,"\"%f\"" ],
    size_t   => [15,"\"%zd\"" ],
    pid_t    => [16,"\"%d\"" ],
    char     => [17,"\"%c\"" ],
    int16_t  => [18 ,"\"%\" PRId16 "],
);

# =========================================================================
if ( $ARGV[0] =~ "^c\$" )
{
    # Print the formatter array and details when this script is executed.
 #   my $format_array = generate_formatter_array();
    my $array_length = scalar keys %type_resolver;
    print "const int csm_min_printable_type = $type_resolver{uint8_t}[0];\n";
    print "const int csm_type_formatter_len = $array_length;\n";
#print "$format_array\n";

}
elsif( $ARGV[0] =~ "^h\$")
{
    print "#include <inttypes.h>\n";
    my $min_printable = $type_resolver{uint8_t}[0];
    my $type_enum = "typedef enum {\n";
    my $macro = "\n#define CSM_PRINT_PRIMATIVE( ptr, post)\\\n";
    my $generic_macro = "\n#define CSM_PRIMATIVE(str, ptr)\\\n";

    for my $key (keys %type_resolver) {
        if ( $type_resolver{$key}[0] < $min_printable )
        {
            $type_enum .= "    CSM_" .uc($key) . "_TYPE = $type_resolver{$key}[0],\n";
           # print "    CSM_" .uc($key) . "_TYPE = $type_resolver{$key}[0],\n";
        }
        else
        {
            $macro .= "    case $type_resolver{$key}[0]: printf($type_resolver{$key}[1] post, *(($key*)ptr));break;\\\n";
            $generic_macro .= "    case $type_resolver{$key}[0]: str.append(std::to_string(*(($key*)ptr))).append(\",\"); break;\\\n";
        }
    }
    $macro .= "\n";
    $type_enum .= "} csmi_type_resolver;\n";
    print $type_enum;
    print $macro;
    print $generic_macro;

}

# =========================================================================

sub resolve_type{
    my $type = shift;
    my $ser_type = shift; 
    my $is_fixed = shift;
    my $is_array = shift;
    my $is_enum  = 0;
    my $computed_type = 0;
    # TODO multidimensional arrays?
    
    # If the type is basic 
    if (  $type_resolver{$type} )
    {
        $computed_type = $type_resolver{$type}[0];
    }
    else
    {
        if ( $type =~ ".*char\*.*" )
        {
            $computed_type = $type_resolver{string}[0];
        }
        elsif ( $ser_type =~ ".*BASIC.*")
        {
            substr($type,-1)=""; # Drop a pointer.
            if (  $type_resolver{$type} )
            {
                $computed_type = $type_resolver{$type}[0];
            }
            else
            {
                $computed_type = $type_resolver{enum}[0];
                $is_enum = 1;
            }
        }
        else
        {
            $computed_type = $type_resolver{misc}[0];
        }
    }
    
    $computed_type = ( $computed_type << 2) | ($is_fixed << 1 ) | $is_array;
    return ($computed_type,$is_enum);
}


sub generate_formatter_array{
    my @type_array = [];
    my $types = "const char* csm_type_formatter[] = {";

    for my $key (keys %type_resolver) {
        my ($idx, $prt) = $type_resolver{$key};
        $type_array[$type_resolver{$key}[0]] = $type_resolver{$key}[1];
    }

    for my $print (@type_array)
    {
        $types .= " $print,";
    }
    substr($types,-1)="};";

    return $types;
}

1;
