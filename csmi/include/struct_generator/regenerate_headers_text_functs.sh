#!/bin/bash
#================================================================================
#
#    csmi/include/regenerate_headers_text_functs.sh
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

# Displays the usage for the regenerate headers function.
usage()
{
cat << EOF
Regenerate Headers Usage -
========================================================================================
   -h           Display this message.
   -c           Clear the version data from the struct *def files.
   -f <#.#.#>   Ships the API for the supplied version number (<Major>.<Minor>.<PTF>).
                    Creates a change log file and updates the md5sums in the type_order
                    files.
========================================================================================
EOF
}

# Builds the copyright header for C.
# $1 - The file name.
build_copyright_header()
{
cat << EOF
/*================================================================================
   
    csmi/$1

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
EOF
}

# Builds the header comment for the struct header.
# $1 - The file name.
# $2 - The name of the struct doxygen group.
build_struct_header_doc()
{
cat << EOF
/** @file $1
 * @brief A collection of structs for @ref $2.
 */
EOF
}

# Builds the header comment for the struct header.
# $1 - The file name.
# $2 - The name of the struct doxygen group.
build_function_header_doc()
{
cat << EOF
/** @file $1
 * @brief A collection of serialization helper functions for @ref $2.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
EOF
}

# Builds defines for the top of the header files.
# $1 - The ifdef guardian definition.
build_struct_defines()
{
cat << EOF
#ifndef $1
#define $1

#ifdef __cplusplus
extern "C" {
#endif

EOF
}

# Builds the end of the header.
build_header_end()
{
cat << EOF

#ifdef __cplusplus
}
#endif
#endif
EOF
}

# Builds the C file include.
# $1 - The function header for the C serialization file.
# $2  - Additional includes
build_c_inc()
{
cat << EOF
#include "$1"

#undef STRUCT_DEF
EOF
}

# Constructs the changelog starting overview.
# $1 - The version code for the changelog.
build_change_log()
{
cat << EOF
# Overview
The following document has been automatically generated to act as a change log for CSM version $1.

# Enum Types
EOF
}
