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
no warnings 'numeric';
use Digest::MD5 qw(md5_hex);
require "./rb_tree.pl"; # A specialized RB tree.
require "./generate_print_formatter.pl"; # Generater for the type to print formatter.

# Constants.
my $INT_DEF  = "CSMI_INTERNAL";
my $COM_DEF  = "CSMI_COMMENT";
my $ALI_DEF  = "CSMI_STRUCT_ALIAS";
my $NAM_DEF  = "CSMI_STRUCT_NAME";
my $MEM_DEF  = "CSMI_STRUCT_MEMBER";
my $VRS_DEF  = "CSMI_VERSION_START";
my $VRE_DEF  = "CSMI_VERSION_END";
my $VRD_DEF  = "CSM_DEVELOPMENT";

my $NFIELDS  = "\n    uint64_t _metadata; /** The number of fields in the struct.*/\n";
my $NULL_C   = "{NULL,0,0,NULL,0,0}";
my $C_MAPPING= "csmi_struct_mapping_t";
my $C_M_NODE = "csmi_struct_node_t";

# File Constants.
my $C_DEF   = "c_funct.txt";
my $H_DEF   = "header_funct.txt";
my $H_A_DEF = "header_funct_alias.txt";

# TODO This takes in too many parameters... 
my $filename       = $ARGV[0]; # The type order file name being processed.
my $output_h_type  = $ARGV[1]; # The type header output file.
my $output_h_funct = $ARGV[2]; # The function header output file.
my $output_c_funct = $ARGV[3]; # The C output file.
my $c_include      = $ARGV[4]; # Includes for the C output file.
my $finalize       = $ARGV[5]; # Flag to indicate 
my $num_fields     = $ARGV[6]; # The number of released fields in the struct.
my $field_code     = $ARGV[7]; # The hash code of the released struct fields.
my $o_int_h_map    = $ARGV[8]; # The internal header file.
my $o_int_c_map    = $ARGV[9]; # The internal C file.
my $o_int_h_types    = $ARGV[10]; # The internal type header file.
my $o_python_classes = $ARGV[11]; # The python classes.
# TODO REDUCE NUMBER OF INPUTS!

my $struct_name = "";        # The name of the struct defined in the file.
my $alias_name  = "";        # The struct that this definition is an alias of.
my $struct_comm = "";        # The comment for the struct.
my $struct_def  = "";        # The def of the struct.
my $field_names = "";        # A string of field names to hold the fields for generating a hash code.
my $field_count = 0;         # The number of fields currently in the struct
my $new_fields  = "";        # The new fields in the struct.
my $tree_root    = undef;    # The root of the mapping tree.
my $version_tree = undef;    # The root of the version tree.
my $py_string    = "";       # The python string.

my $version_id = 1;          # The version id.
my $version_str= "";         # The version string for docuementation.
my $temp_hash  = "";
my $file_swp   = undef;

open( my $file, "<", $filename ) or die "Can't open $filename : $!";

if ( $finalize eq 1 ) {
    open( $file_swp, ">", "$filename.swp") or die "Can't open $filename.swp : $!";
}

while ( <$file> ) {
    if(/#define/){
        chomp; 
        # Extract then assign appropriately
        $_ =~ /#define\s+(\w*)\s+(\w*)/;

        my $value = $2;
        if    ($1 =~ $ALI_DEF) { $alias_name  = $value; }
        elsif ($1 =~ $NAM_DEF) { $struct_name = $value; }
    }   
    elsif (/$MEM_DEF/){
        chomp; 
        # TODO simplify!
        $_ =~ /^CSMI_STRUCT_MEMBER\(\s*([^ ,]*)[^,]*,\s*([^ ,]*)[^,]*,\s*([^ ,]*)[^,]*,\s*([^ ,]*)[^,]*,\s*([^ ,]*)[^,]*,\s*([^\) ]*)\s*\)([\/\*\<]*.*)/;

        if( defined $1 ) { 
            my $type     = $1;
            my $name     = $2;
            my $ser_type = $3;
            my $len_mem  = $4;
            my $init_val = $5;
            my $extra    = $6;
            my $comment  = $7;

            $field_names .= $name;
            $field_names .= $type;
            $field_count++;

            if ( $field_count == $num_fields && 
                    $field_code != -1        && 
                    $field_code ne md5_hex($field_names))
            {
                print("ERROR: Released struct field has been modified improperly! ");
                print("Indicates the struct field was renamed, reordered or had its type changed.");
                exit 1;
            }

            # Hash Table helpers
            my $is_fixed    = 0;
            my $is_array    = 0;
            my $size_offset = "0";
            my $fields      = "NULL";
            #$type =~ /([^\*]*)?[\*\[]/;
            #my $field_size = "sizeof($1)";

            if($ser_type =~ ".*ARRAY.*") 
            { 
                $is_array = 1;
            }

            # If this is a fixed add the length member in the definition.
            # Else just add the struct.
            if ( $ser_type =~ ".*FIXED.*" ) 
            {
                $struct_def .= "    $type $name\[$len_mem\];$comment\n";
                
                if ( $finalize eq 1 && $field_count > $num_fields )
                {
                    $new_fields .= ";* $type $name\[$len_mem\] ";
                }
                $is_fixed = 1;
                $size_offset = $len_mem;
            }
            else
            {
                $struct_def .= "    $type $name;$comment\n";
                
                if ( $finalize eq 1 && $field_count > $num_fields )
                {
                    $new_fields .= ";* $type $name ";
                }
                if( $is_array == 1) { $size_offset = "offsetof($struct_name, $len_mem)";}
            }
            
            # If this is a struct field add the hashing information.
            if ( $ser_type =~ ".*STRUCT.*" ) { $fields = "&map_$extra"; }

            
            # Resolves the type for specifying the print behavior for the field.
            my ($computed_type,$is_enum) = resolve_type($type,$ser_type,$is_fixed,$is_array);
            
            # Enumerated types have special behavior.
            if ( $is_enum ) 
            { 
                $size_offset = "${type}_MAX";
                $fields = "&${type}_strs"; 
            }

            # DJB2 hashing has counterpart in C code. 32 bit
            my $hash = 5381;
            
            foreach my $char (split //, $name) {
                $hash = ((( $hash << 5 ) + $hash) + ord($char))  & 0xFFFFFFFF;
            }

            # Generate the entry for the tree.
            my $hash_entry = sprintf("{\"$name\",offsetof($struct_name,$name),$size_offset,$fields,0x%x,$computed_type}", $hash);
            InsertRBNode( $tree_root, $hash, $hash_entry);
            # End HASH
            
            # Start Python
            # TODO Do Better!
            #$comment =~ /\/\*\*<(.*)*\*\//;

            #$py_string .= "\n\t\t.add_property(\"$name\",make_getter(&${struct_name}::$name, return_value_policy<reference_existing_object>()),make_setter(&${struct_name}::$name, return_value_policy<reference_existing_object>()),  \"$comment\")";
            if ( $is_fixed == 0 )
            {
                $comment =~ /\/\*\*<(.*)\*\//;
                my $stripped_comment = $1;
                if (${ser_type} eq "BASIC")
                {
                    $py_string .= "\n\t\t.add_property(\"${name}\", &${struct_name}::$name,&${struct_name}::$name,\"$stripped_comment\")"
                }
                elsif(${ser_type} eq "STRUCT")
                {
                    $py_string .= "\n\t\tSTRUCT_PROPERTY(${struct_name}, ${type}, ${name}, ${len_mem}, ${init_val}, &${struct_name}::$name)"
                }
                else
                {
                    $py_string .= "\n\t\t${ser_type}_PROPERTY(${struct_name}, ${type}, ${name}, ${len_mem}, ${init_val}, ${extra})";
                }
                #$py_string .= "\n\t\t.add_property(\"$name\",&${struct_name}::$name,&${struct_name}::$name,\"\")";
            }
        }
    }
    elsif(/$COM_DEF/ .. /\*\//){
        # Ignore surrounding code.
        if(!/$COM_DEF/ and !/\*\//) { $struct_comm .= "$_"; }
        chomp;
    }
    elsif (/$VRS_DEF\(/) # Process the start of a version block.
    {
        chomp; 
        $_ =~ /^$VRS_DEF\(.*_(\d+)_(\d+)_(\d+).*\)/;
        
        if ( defined $1 )
        {
            $version_id = ($1 << 16) + ( $2 << 8 ) + $3;
        }
        else
        {
            if ( $_ =~ ".*$VRD_DEF.*" )
            {
                $version_id = 0;
            }
            else
            {
                print("ERROR: Invalid version block detected: $_"); 
                exit 1;
            }
        }
    }
    elsif (/$VRE_DEF\(/) # Process the end of a version block.
    {
        chomp; 
        $_ =~ /^$VRE_DEF\([ ]*([\da-f]*)[ ]*\).*/;

        # If a valid version was supplied and the version id is non zero, test it.
        if ( defined $1 && $version_id gt 0 )
        {
            if ( md5_hex($field_names) ne $1 ) 
            {
                $temp_hash=md5_hex($field_names);
                print("ERROR: Modifications were detected in the $version_id version block.");
                print("Indicates a struct field was renamed, reordered or had its type changed.");
                exit 1;
            }
        }
        elsif( $version_id eq 0 &&  $finalize eq 1 )
        {
            $temp_hash=md5_hex($field_names);
            say $file_swp "$VRE_DEF($temp_hash)";
            next;
        }
        elsif ( $version_id gt 0 )
        {
            print("ERROR: Version was not detected to be development and there was no metadata.");
            print("  version id : $version_id");
            print("Line: $_");
            exit 1;
        }
    }
    elsif(/$INT_DEF/){
        $output_h_type  = $o_int_h_types;
        $output_h_funct = $o_int_h_types;
        $o_python_classes = "/dev/null";
        chomp;
    #    $output_c_funct = $o_int_c_map;
    }
    else
    {
        chomp;
    }
    
    if ( $finalize eq 1 )
    {
        say $file_swp "$_";
        next;
    }
}

close $file;

# On a finalize, rename the swap file.
if ( $finalize eq 1 )
{
    close $file_swp;
    rename "$filename.swp", "$filename"
}


if ( $field_count < $num_fields && 
        $field_code != -1        && 
        $field_code ne md5_hex($field_names))
{
    print("ERROR: Released struct field has been modified improperly! ");
    print("Detected that struct fields were removed!");
    exit 1
}

# Construct the headers for the struct.
#=============================================================================
my $struct = << "STRUCT";
/**
$struct_comm */
STRUCT
#/**< The number of fields in \@ref ${struct_name} */
#define ${struct_name}_NUM_FIELDS ${num_fields}
#/** \@defgroup ${struct_name}  ${struct_name} */
#/** \@ingroup ${struct_name}


# If the alias is not set build the header and c file.
# Else build just the alias.
if ( $alias_name  eq "" )
{
    $struct .=  "typedef struct {${NFIELDS}${struct_def}} ${struct_name};";
    
    # Write the contents of the struct
    open(my $o,">>",$output_h_type ) or die "Could not open $output_h_type";
    say $o "$struct";
    close $o;

    my $md5_sum = substr(md5_hex(${struct_name}), 0, 8);
    copy_file(${H_DEF}, ${output_h_funct}, " ", ${struct_name}, ${alias_name});
    copy_file(${C_DEF}, ${output_c_funct}, "#define STRUCT_DEF \"${c_include}\"\n#define STRUCT_SUM 0x${md5_sum}",
        ${struct_name}, ${alias_name});
}
else
{
    $struct .=  "typedef ${alias_name} ${struct_name};";
    
    # Write the contents of the struct
    open(my $o,">>",$output_h_type ) or die "Could not open $output_h_type";
    say $o "$struct";
    close $o;

    copy_file(${H_A_DEF}, ${output_h_funct}, " ", ${struct_name}, ${alias_name});
}
#=============================================================================

# If the finalize flag is set print the field codes.
if( $finalize eq 1 )
{
    if ( $alias_name eq "" ){
        $field_code = md5_hex($field_names);
        print(" ${field_count} ${field_code} ${struct_name} $new_fields");
    }
    else
    {
        print(" -1 -1 ${struct_name}\n");
    }
}

#=============================================================================
# Finalize the python definition.
if ( $alias_name  eq "" )
{
    $_ = $struct_name;
    s/^[^_]*_//;
    my $py_class_full = <<"PY_CLASS";
    class_<$struct_name,$struct_name*>("$_")$py_string;
PY_CLASS

    open(my $opc,">>",$o_python_classes ) or die "Could not open $o_python_classes";
    say $opc "$py_class_full";
    close $opc;
}


#=============================================================================
# Build the struct mapping.
my $const_name = "map_$struct_name";
my $const_tree = "${struct_name}ree";
#my $mapping_struct = "const $C_MAPPING* $const_name";
my $mapping_tree   = "const ${C_M_NODE} ${const_tree}";
my $mapping_struct = "const $C_MAPPING $const_name";
my $cast_funct     = "";
my $mapping_definition = << "DEF";
extern $mapping_struct;
DEF

# If the struct isn't an alias
if ( $alias_name  eq "" )
{
    my ($tree_size,$tree_string) = RBArrayOrder($tree_root, $NULL_C);


    $mapping_tree .= << "TREE";
[$tree_size] = $tree_string;

TREE

    # Construct the cast function that will be used to dereference arrays.
#    $cast_funct = << "FUNCT";
#void* cast_${struct_name}(void* ptr,size_t index) { return ((${struct_name}**)*(${struct_name}**)ptr)[index];}
#FUNCT
    $cast_funct = << "FUNCT";
void* cast_${struct_name}(void* ptr,size_t index) { 
    ${struct_name} ** ptr_cast = *(${struct_name}***)ptr;
    return ptr_cast ? ptr_cast[index] : NULL;
};
FUNCT
#void* malloc_${struct_name}(size_t num) {
#    return (${struct_name}*) malloc(sizeof(${struct_name}) * num);
#};

    $mapping_struct .= << "STRUCT";
= {
    $tree_size,
    $const_tree,
    cast_${struct_name}
};
STRUCT
}
else 
{ 
    $mapping_tree = "";
    $mapping_struct = "";
    $mapping_definition = "#define $const_name map_$alias_name";
}

# Write to the internal header.
open(my $omh,">>",$o_int_h_map ) or die "Could not open $o_int_h_map";
say $omh "$mapping_definition";
close $omh;

# Write to the internal c file.
open(my $omc,">>",$o_int_c_map ) or die "Could not open $o_int_c_map";
say $omc "$mapping_tree$cast_funct$mapping_struct";
#say $omc "$mapping_tree";
close $omc;
#=============================================================================

# Copies the files.
sub copy_file {
    my $file_name_i = shift;
    my $file_name_o = shift;
    my $prefix      = shift;
    my $struct_name = shift;
    my $alias_name  = shift;

    open( my $file, "<", $file_name_i ) or die "Could not open $output_h_type";

    # Start the output with the prefix
    my $output = ${prefix};
    
    # Iterate over the input file and replace any instances of struct or alias name.
    while ( <$file> ) {
        my $line = $_;
        $line   =~ s/REPLACE_STRUCT_NAME/$struct_name/g;
        $line   =~ s/ALIAS_NAME/$alias_name/g;
        $output .= $line;
    }
    close $file;
    
    # Write the contents of the cleaned file.
    open(my $o,">>",$file_name_o ) or die "Could not open $file_name_o";
    say $o "$output";
    close $o;
}   

