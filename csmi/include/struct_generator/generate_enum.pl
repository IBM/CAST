#!/bin/perl
#================================================================================
#
#    csmi/include/generate_enum.pl
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

# Constants.
my $COM_DEF  = "CSMI_ENUM_BRIEF";
my $BIT_DEF  = "CSMI_BIT_ENUM";
my $NAM_DEF  = "CSMI_ENUM_NAME";
my $MEM_DEF  = "CSMI_ENUM_MEMBER";
my $VRS_DEF  = "CSMI_VERSION_START";
my $VRE_DEF  = "CSMI_VERSION_END";
my $VRD_DEF  = "CSM_DEVELOPMENT";

my $filename      = $ARGV[0];
my $output_header = $ARGV[1];
my $output_c      = $ARGV[2];
my $finalize      = $ARGV[3];
my $num_enums     = $ARGV[4]; # The released enum count total.
my $enum_code     = $ARGV[5]; # The hash code of the released enum values.
my $o_python      = $ARGV[6]; # The python enum types.

my $use_bit     = 0;
my $enum_prefix = "";
my $bit_count   = 0;
my $bit_def     = "";
my $enum_val    = 0;
my $enum_name   = "";
my $enum_comm   = "";
my $enum_names  = "";
my $enum_count  = 0;
my $active_enum_code = -1;

my $enum_def  ="";
my $enum_array="";
my $new_fields = "";
my $version_id = 1;          # The version id.
my $version_str= "";         # The version string for docuementation.
my $temp_hash  = "";
my $file_swp   = undef;
my $py_string  = "";         # The python string.

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
        if    ($1 =~ $BIT_DEF) { $enum_val = 1; $use_bit=1; $enum_prefix = $value; }
        elsif ($1 =~ $NAM_DEF) { $enum_name = $value;}
    }   
    elsif (/$MEM_DEF/){
        chomp;
        # TODO simplify!
        $_ =~ /^CSMI_ENUM_MEMBER\(\s*(\w*)[^,]*,[^"]*("[^"]*")[^,]*,\s*([^ ,]*)[^,]*,\s*([^\) ]*)\s*\)[\/\*\< ]*(.*)/;

        if( defined $1 ) { 
            my $name= $1;
            my $str = $2;
            my $val = $3;
            my $com = $5;

            $enum_names .= $name;
            $enum_names .= $val;
            $enum_count++;
            
            if ( $enum_count == $num_enums &&
                    $enum_code != -1       &&
                    $enum_code ne md5_hex($enum_names))
            {
                print("ERROR: Released enumerated values have been modified improperly! ");
                print("Indicates that an enumerated value was renamed, reordered or its value was changed.");
                exit 1;
            }

            if( !$use_bit and $3 =~ /[0-9]+/ ) { $enum_val = $val; }

            $enum_def   .= "   ${name}=${enum_val}, ///< ${enum_val} - $com\n"; 
            $enum_array .= "$str,"; 
            
            # Add a new line every 10 values.
            if( ( $enum_count % 10 ) == 0) { $enum_array = $enum_array . "\n";}
            
            # If finalizing generate a new listing.
            if ( $finalize eq 1 && $enum_count > $num_enums )
            {
                $new_fields .= ";* ${name}=${enum_val}";
            }
            
            $py_string .= "\n\t\t.value(\"${name}\",${name})";

            if($use_bit) { 
                $enum_val = $enum_val * 2;
                $bit_count++;
            }
            else {
                $enum_val++;
            }
        }
    }
    elsif(/$COM_DEF/ .. /\*\//){
        # Ignore surrounding code.
        if(!/$COM_DEF/ and !/\*\//) {
             $enum_comm .= $_;
        }
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
            if ( md5_hex($enum_names) ne $1 ) 
            {
                $temp_hash=md5_hex($enum_names);
                print("ERROR: Modifications were detected in the $version_id version block.");
                print("Indicates an enum value was renamed, reordered or had its type changed.");
                exit 1;
            }
        }
        elsif( $version_id eq 0 &&  $finalize eq 1 )
        {
            $temp_hash=md5_hex($enum_names);
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

# Python
#$_ = $enum_name;
#s/^[^_]*_//;
my $py_enum_full = << "PY_ENUM";
    enum_<$enum_name>("$enum_name")$py_string;
PY_ENUM

open(my $opc,">>",$o_python ) or die "Could not open $o_python";
say $opc "$py_enum_full";
close $opc;

# On a finalize, rename the swap file.
if ( $finalize eq 1 )
{
    close $file_swp;
    rename "$filename.swp", "$filename"
}

# Verify that the enum was not changed improperly (catches the edge case).
if ( $enum_count < $num_enums &&
        $enum_code != -1       &&
        $enum_code ne md5_hex($enum_names))
{
    print("ERROR: Released enumerated values have been modified improperly! ");
    print( "Detected that enumerations values were removed!");
    exit 1;
}

# IF this is a bit flag
if ( $use_bit ) {
    # Add NONE 
    $enum_def   = "   ${enum_prefix}NONE=0, ///< 0 - No flags set\n${enum_def}";
    $enum_array = "\"NONE\", ". ${enum_array};

    my $all_idx = $enum_val - 1;
    #Add ALL
    $enum_def   = "${enum_def}   ${enum_prefix}ALL=$all_idx, ///< $all_idx - All flags set\n";
    $enum_array = ${enum_array} . "\"ALL\",";

    $bit_def = "#define ${enum_name}_bit_count ${bit_count}"
}

# Add the limiter to the enumerated type.
#my $temp = "${enum_def}";
$enum_def = "${enum_def}   ${enum_name}_MAX=${enum_val} ///< ${enum_val} - Bounding Value";

# Build the array string.
$enum_array="const char* ${enum_name}_strs [] = {$enum_array\"\"};";

# Build the struct
my $struct = << "ENUM_START";
/** defgroup $enum_name $enum_name
 * \@{
 */
/**
 $enum_comm  */
typedef enum {
$enum_def
} $enum_name;

$bit_def

/**
 * \@brief Maps enum $enum_name value to string.
 */
extern const char* ${enum_name}_strs [];
/** \@}*/
ENUM_START

# Write to the files.
open(my $oh,">>",$output_header ) or die "Could not open $output_header";
say $oh "$struct\n";
close $oh;

open(my $oc,">>",$output_c ) or die "Could not open $output_c";
say $oc "$enum_array\n";
close $oc;

if ( $finalize eq 1 )
{
    $enum_code = md5_hex($enum_names);
    print(" ${enum_count} ${enum_code} ${enum_name} ${new_fields}");
}
