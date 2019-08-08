/*================================================================================
   
    csmi/src/wm/src/csmi_wm_serialization.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/include/csmi_type_wm_funct.h"

#undef STRUCT_DEF
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
const char* csmi_state_t_strs [] = {"staging-in","to-running","running","to-staging-out","staging-out","to-complete","complete","to-failed","failed","deleting",
"deleting-mcast","running-failed","staging-out-failed",""};

const char* csmi_cgroup_controller_t_strs [] = {"cpuset","memory","devices","cpuacct",""};

const char* csmi_allocation_type_t_strs [] = {"user-managed","jsm","jsm-cgroup-step","diagnostics","cgroup-step",""};

const char* csmi_job_type_t_strs [] = {"batch","interactive",""};

const char* csmi_step_status_t_strs [] = {"running","completed","killed",""};

#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_history.def"
#define STRUCT_SUM 0x140ab6cb
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation.def"
#define STRUCT_SUM 0x13a86231
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_accounting.def"
#define STRUCT_SUM 0xbb8fa4d7
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_step_list.def"
#define STRUCT_SUM 0x8d0cbc56
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_state_history.def"
#define STRUCT_SUM 0xbb89762d
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_details.def"
#define STRUCT_SUM 0x60f39275
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_step_history.def"
#define STRUCT_SUM 0x2604775a
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_step.def"
#define STRUCT_SUM 0xd7905381
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_ssd_resources_record.def"
#define STRUCT_SUM 0x988b4548
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_node_resources_record.def"
#define STRUCT_SUM 0x7b18f280
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_cgroup.def"
#define STRUCT_SUM 0xf430c40d
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_resources_record.def"
#define STRUCT_SUM 0xb9238f74
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_details_input.def"
#define STRUCT_SUM 0xb78f96a2
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_details_output.def"
#define STRUCT_SUM 0xfbcd04f8
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_update_state_input.def"
#define STRUCT_SUM 0xea496f1b
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_end_input.def"
#define STRUCT_SUM 0xbafd2527
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_query_input.def"
#define STRUCT_SUM 0xacd9938b
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_query_output.def"
#define STRUCT_SUM 0x62517485
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_query_details_input.def"
#define STRUCT_SUM 0xf6d23071
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_query_active_all_input.def"
#define STRUCT_SUM 0xc00191cd
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_node_resources_query_input.def"
#define STRUCT_SUM 0x49606b4a
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_node_resources_query_output.def"
#define STRUCT_SUM 0xf451998b
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_node_resources_query_all_input.def"
#define STRUCT_SUM 0x7296a9d1
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_node_resources_query_all_output.def"
#define STRUCT_SUM 0xfa34fef2
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_cgroup_create_input.def"
#define STRUCT_SUM 0x11619b09
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_step_cgroup_delete_input.def"
#define STRUCT_SUM 0x0ed19130
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_resources_query_input.def"
#define STRUCT_SUM 0x06d0c617
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_resources_query_output.def"
#define STRUCT_SUM 0x423ea695
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_update_history_input.def"
#define STRUCT_SUM 0x1e280ea5
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_input.def"
#define STRUCT_SUM 0xb5e80ccc
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_output.def"
#define STRUCT_SUM 0xce8c3f6c
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_active_all_input.def"
#define STRUCT_SUM 0xe891ea52
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_query_active_all_output.def"
#define STRUCT_SUM 0xa7a53b58
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_allocation_delete_input.def"
#define STRUCT_SUM 0x123c30f6
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_cgroup_login_input.def"
#define STRUCT_SUM 0x4774836d
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_gpu_metrics.def"
#define STRUCT_SUM 0xdb569bbb
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_mcast_context.def"
#define STRUCT_SUM 0x12421ecd
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_mcast_payload_request.def"
#define STRUCT_SUM 0xcf6373b0
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_mcast_payload_response.def"
#define STRUCT_SUM 0x6a94218c
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_step_mcast_context.def"
#define STRUCT_SUM 0xebdbd907
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_allocation_step_mcast_payload.def"
#define STRUCT_SUM 0xb2dd7599
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_jsrun_cmd_payload.def"
#define STRUCT_SUM 0x81258bd8
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_jsrun_cmd_input.def"
#define STRUCT_SUM 0x3421b09a
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csmi_soft_failure_recovery_payload.def"
#define STRUCT_SUM 0x466f4649
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_soft_failure_recovery_node.def"
#define STRUCT_SUM 0xb75fca32
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_soft_failure_recovery_input.def"
#define STRUCT_SUM 0xb7c1a2ea
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/wm/csm_soft_failure_recovery_output.def"
#define STRUCT_SUM 0x14a422c9
#define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)
#define CSMI_VERSION_START(version)
#define CSMI_VERSION_END(hash)
#include STRUCT_DEF
LEN_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return 0;

    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;
    #endif

    uint32_t len = 0;
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        LEN_##serial_type(type, name, length_member, metadata);                              
    
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    return len;
}

PACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the target is null, don't even bother trying to pack it.
    if(!target) return;

    #if CSMI_STRING || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT || CSMI_ARRAY
    uint32_t size_dump = 0;           //< A dump variable to store sizes.
    #endif
    #if CSMI_ARRAY ||  CSMI_ARRAY_FIXED || CSMI_ARRAY_STR || \
    CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED  
    uint32_t i = 0;                   //< A counter variable.
    #endif

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        PACK_##serial_type(type, name, length_member, metadata);                         

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF
}

SERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    const uint64_t struct_magic = STRUCT_SUM; //< The md5 sum of the struct, used for authentication.
    uint32_t size = 1;                        //< The number of elements being serialized.
    uint64_t version_id  = target->_metadata; //< The metadata object for the struct.
    uint32_t len  = sizeof(uint64_t) + UINT32_T_SIZE + 
                            UINT64_T_SIZE;   //< Length of the buffer, start with size offset.

    char    *buffer = NULL;              //< The buffer that will hold the packing results.
    uint32_t offset = 0;                 //< The offset of the pack so far.
    int      ret_code = 0;

    // If the version id is above the MIN Version
    if ( version_id >= CSM_MIN_VERSION ) 
    {
        // Get the buffer length. 
        len += CSM_FUNCT_CAT( len_, CSMI_STRUCT_NAME )(target, version_id);

        // Allocate the buffer.
        buffer = (char*) calloc(len, sizeof(char));
    
        //Add the magic bytes to the front of the struct.
        memcpy( buffer, &struct_magic, sizeof(uint64_t) );
        offset = sizeof(uint64_t);

        // Copy the version info of the struct.
        memcpy(buffer+offset, &version_id, UINT64_T_SIZE);
        offset += UINT64_T_SIZE;

        // Copy the buffer starting with the number of structs present.
        memcpy(buffer + offset, &size, UINT32_T_SIZE);
        offset += UINT32_T_SIZE;
        
        CSM_FUNCT_CAT( pack_, CSMI_STRUCT_NAME )(target, buffer, &offset, len, version_id);

        // If the offset exceeds the length and somehow didn't seg fault throw an exception, update the ret_code.
        ret_code = offset == len ? 2 : 0;
        
        // Save the final values.
        *buffer_len = len; 
        *buf        = buffer;
    }
    else
    {
        *buffer_len = 0;
        *buf = NULL;
        ret_code = 1;
    }

    return ret_code;
}

UNPACK_FUNCT(CSMI_STRUCT_NAME)
{
    // EARLY RETURN!
    // If the buffer is null or zero length, don't even bother trying to unpack it.
    if( !(buffer && len && *offset <= len) ) 
    {
        *dest = NULL; 
        return;
    }

    #if CSMI_STRING || CSMI_ARRAY || CSMI_ARRAY_STR || CSMI_STRUCT || CSMI_ARRAY_STRUCT 
    uint32_t size_dump = 0;             //< A dump variable to store sizes.
    #endif

    #if CSMI_ARRAY || CSMI_ARRAY_FIXED || CSMI_ARRAY_STR ||\
        CSMI_ARRAY_STR_FIXED || CSMI_ARRAY_STRUCT || CSMI_ARRAY_STRUCT_FIXED
    uint32_t i = 0;                     //< A counter variable.
    #endif

    #if CSMI_ARRAY_STR  || ARRAY_STR_FIXED
    uint32_t str_offset = 0;            //< An additional offset for a string in a loop.
    #endif

    CSMI_STRUCT_NAME *target;  //< The destination,the target will be unpacked to.
    target = (CSMI_STRUCT_NAME*) calloc(1, sizeof(CSMI_STRUCT_NAME));

    // Unpack IFF the field has been prepared, else initialize to the default.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        UNPACK_##serial_type(type, name, length_member, metadata);

    #define CSMI_VERSION_START(version) if ( version_id >= version ) {
    #define CSMI_VERSION_END(hash) }
    #include STRUCT_DEF

    // Test the offeset against the length
    if ( *offset <= len )
    {
        // If the version id is not equal to released version, test for initialization.
        if ( version_id < CSM_VERSION_ID )
        {
            #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
                INIT_##serial_type(name, init_value, length_member)

            #define CSMI_VERSION_START(version) if ( version_id < version ) {
            #define CSMI_VERSION_END(hash) }
            #include STRUCT_DEF
        }
    }
    else
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, target);
        target = NULL;
    }

    *dest = target;
}

DESERIALIZE_FUNCT(CSMI_STRUCT_NAME)
{
    // If the destination is not defined or the buffer doesn't contain the struct count return.
    if( !dest || buffer_len < UINT32_T_SIZE )
    {
        if(dest) *dest = NULL;
        return 1;
    }

    uint64_t struct_magic  = 0;  //< The md5 sum of the struct, used for authentication.
    uint32_t offset        = 0;  //< The offset for the buffer.
    uint32_t structs_found = 0;  //< The number of structs found in the serialization.
    uint64_t version_id    = 0;  //< The version of the supplied struct.

    // Retrieve the magic bytes from the front of the struct.
    memcpy( &struct_magic, buffer, sizeof(uint64_t) );
    offset = sizeof(uint64_t);

    // Extract the version id of the struct.
    memcpy(&version_id, buffer + offset, UINT64_T_SIZE);
    offset += UINT64_T_SIZE;

    // The number of elements always leads the buffer.
    memcpy(&structs_found, buffer + offset, UINT32_T_SIZE);
    offset += UINT32_T_SIZE;
    
    // Verify the buffer has any structs AND the struct sum is valid.
    if (structs_found == 1 && struct_magic == STRUCT_SUM)
    {
        CSM_FUNCT_CAT( unpack_, CSMI_STRUCT_NAME )( dest, buffer, &offset, buffer_len, version_id);
    }
    else
    {
        *dest = NULL;
        return 1;
    }
    
    // Fail buffer overflows.
    if(offset > buffer_len)
    {
        csm_free_struct_ptr(CSMI_STRUCT_NAME, *dest);
        return 1;
    }

    // Cache the version id in the metadata field.
    if ( *dest ) (*dest)->_metadata = version_id;

    return 0;
}

FREE_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return;    //< If the target is null return.
    #if CSMI_ARRAY_STR || CSMI_ARRAY_STR_FIXED || \
        CSMI_ARRAY_STRUCT_FIXED || CSMI_ARRAY_STRUCT
    uint32_t i = 0;             //< A counter variable.
    #endif

    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_ARRAY_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF

    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata) \
        FREE_##serial_type(type, name, length_member, metadata)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}

INIT_FUNCT(CSMI_STRUCT_NAME)
{
    if (!target) return; //< If the target is null return. TODO Error throwing.
    
    // Get the version the user is supplying.
    uint64_t version_id    = target->_metadata <= CSM_VERSION_ID ? target->_metadata : CSM_MIN_VERSION;

    
    // Initialize the members.
    #define CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, metadata)\
        INIT_##serial_type(name, init_value, length_member)
    #define CSMI_VERSION_START(version) if (version_id >= version ) {
    #define CSMI_VERSION_END(hash) } 
    #include STRUCT_DEF
}
#undef STRUCT_DEF
#undef STRUCT_SUM
#undef CSMI_STRUCT_NAME



