/*================================================================================
   
    csmi/src/inv/src/csmi_inv_serialization.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/include/csmi_type_inv_funct.h"

#undef STRUCT_DEF
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmi/src/inv/include/csmi_inv_type_internal.h"
const char* csmi_node_alteration_t_strs [] = {"undefined","CSM_DATABASE_NULL","CSM API","RAS EVENT","SYSTEM ADMINISTRATOR","CSM INVENTORY",""};

#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_dimm_record.def"
#define STRUCT_SUM 0xeb02aea7
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_gpu_record.def"
#define STRUCT_SUM 0x51378a01
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_hca_record.def"
#define STRUCT_SUM 0xe97d6932
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_ib_cable_record.def"
#define STRUCT_SUM 0x74ceab82
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_ib_cable_history_record.def"
#define STRUCT_SUM 0xf3d5948c
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_attributes_record.def"
#define STRUCT_SUM 0x43c2795d
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_attributes_history_record.def"
#define STRUCT_SUM 0x98a498f9
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_query_state_history_record.def"
#define STRUCT_SUM 0x3e382487
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_processor_record.def"
#define STRUCT_SUM 0x63bb9fec
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_ssd_record.def"
#define STRUCT_SUM 0x997bfadf
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_record.def"
#define STRUCT_SUM 0xfb63c39f
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_inventory_record.def"
#define STRUCT_SUM 0x442c1b02
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_ports_record.def"
#define STRUCT_SUM 0xe27c3e26
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_details.def"
#define STRUCT_SUM 0x7db6f656
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_history_record.def"
#define STRUCT_SUM 0xe6628868
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_env_data.def"
#define STRUCT_SUM 0x29ff8335
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_switch_env_data.def"
#define STRUCT_SUM 0x78fd10b3
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_fabric_topology.def"
#define STRUCT_SUM 0x1600a9bd
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_details.def"
#define STRUCT_SUM 0x6bc11d5e
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_cluster_query_state_record.def"
#define STRUCT_SUM 0x91a89bf7
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csmi_node_find_job_record.def"
#define STRUCT_SUM 0x64ce3bf8
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_inventory_collection_input.def"
#define STRUCT_SUM 0xd5701e2e
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_inventory_collection_output.def"
#define STRUCT_SUM 0xb807aaa9
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_query_input.def"
#define STRUCT_SUM 0x040b9492
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_query_output.def"
#define STRUCT_SUM 0x06527c4e
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_query_history_input.def"
#define STRUCT_SUM 0x87f78799
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_query_history_output.def"
#define STRUCT_SUM 0x409527c6
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_update_input.def"
#define STRUCT_SUM 0x5af92e73
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_ib_cable_update_output.def"
#define STRUCT_SUM 0xf6567f5a
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_input.def"
#define STRUCT_SUM 0x7232b69d
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_output.def"
#define STRUCT_SUM 0xd715989b
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_details_input.def"
#define STRUCT_SUM 0xa52fdfa8
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_details_output.def"
#define STRUCT_SUM 0xf160e454
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_history_input.def"
#define STRUCT_SUM 0x93061b51
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_query_history_output.def"
#define STRUCT_SUM 0x4ba074ec
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_query_state_history_input.def"
#define STRUCT_SUM 0xec80d714
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_query_state_history_output.def"
#define STRUCT_SUM 0xa99cc56e
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_update_input.def"
#define STRUCT_SUM 0x1773fa5e
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_attributes_update_output.def"
#define STRUCT_SUM 0x5c0349a4
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_delete_input.def"
#define STRUCT_SUM 0x965ccd2f
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_delete_output.def"
#define STRUCT_SUM 0x09080062
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_find_job_input.def"
#define STRUCT_SUM 0xad5fa8fa
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_node_find_job_output.def"
#define STRUCT_SUM 0xf013d68c
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_input.def"
#define STRUCT_SUM 0x3d189cb9
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_output.def"
#define STRUCT_SUM 0xc8f44323
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_details_input.def"
#define STRUCT_SUM 0x72e3bcee
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_details_output.def"
#define STRUCT_SUM 0xc41d9c20
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_history_input.def"
#define STRUCT_SUM 0x7314d731
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_query_history_output.def"
#define STRUCT_SUM 0xf3ef3656
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_update_input.def"
#define STRUCT_SUM 0xd1877483
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_attributes_update_output.def"
#define STRUCT_SUM 0xcfcb8570
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_inventory_collection_input.def"
#define STRUCT_SUM 0xe6348b90
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_inventory_collection_output.def"
#define STRUCT_SUM 0x4c2f9e66
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_children_inventory_collection_input.def"
#define STRUCT_SUM 0xa3a58ed8
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_switch_children_inventory_collection_output.def"
#define STRUCT_SUM 0xcf128483
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_cluster_query_state_input.def"
#define STRUCT_SUM 0xf74c1b8f
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/inv/csm_cluster_query_state_output.def"
#define STRUCT_SUM 0xf923293c
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



