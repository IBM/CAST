/*================================================================================

    csmi/include/csm_types/enum_defs/sample_enum.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

// Struct metadata for the preprocessor.
#ifndef CSMI_ENUM_NAME
    // Define the name of the allocation.
    #define CSMI_ENUM_NAME @ENUM_NAME
    // CSMI_ENUM_BRIEF Comment about enum goes here.
    
#endif 
// Comments need to be on the same line!
// CSMI_ENUM_MEMBER( enum_member, member_string, enum_num, relationship)
CSMI_ENUM_MEMBER(STAGING_IN, "staging in",, )   ///< The allocation is staging out.

#undef CSMI_ENUM_MEMBER
