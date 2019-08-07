/*================================================================================
   
    csmi/src/common/src/csmi_common_serial.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/include/csmi_type_common_funct.h"

#undef STRUCT_DEF
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmi/src/common/include/csmi_common_type_internal.h"
const char* csmi_cmd_err_t_strs [] = {"NO ERROR","An undefined error was detected.","A help functionality caused an early exit.","CSM Library not initialized","No results received","Timeout","Message Id mismatched","CSMI CMD mismatched","Missing required parameter","Invalid parameter or value",
"Ras handler exception","CSMI CMD Unknown To Daemon","Send or Recv Error","Memory error","Not defined error","PubSub error","CSMI permission denied","Script failure error","Can not connect to daemon","Can not disconnect from daemon",
"The Payload of a message was unexpectedly empty","Handler received incorrect event type","The Address type of a network message was unexpected","Indicates that a CGroup couldn't be deleted","Database error; can't connect","Database error; table in bad state","Number of deleted records is less than expected","Number of updated records is less than expected","Message packing error","Message unpack error",
"The String Buffer of a csmi_sendrecv_cmd message was unexpectedly empty","The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty","The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted","It was not possible to create the multicast message","Errors were found with the responses from the compute daemons","API has a bad permission level.","A generic error occurred modifying cgroups.","The handler context was lost in some way.","An invalid value was written to a cgroup parameter.","An illegal resource request occurred for the cgroup parameter.  ",
"Burst Buffer Command encountered a generic error.","The allocation jumped from staging-in to staging-out.","The allocation state transition was to the same state.","The allocation state transition failed to complete.","The allocation delete was performed in an illegal state.","Indicates JSRUN could not be started by the CSM infrastructure.","Nodes specified for allocation create were not in the database.","Nodes specified for allocation create were in use by other allocations.","Allocation create had nodes that were not available.","Allocation create had bad allocation flags.",
"Allocation couldn't find the allocation.","Indicates an epilog collided with an epilog.","Indicates an epilog collided with a prolog.","Indicates a prolog collided with an epilog.","Indicates a prolog collided with a prolog.","Indicates a generic error in the soft failure recovery agent.",""};

const char* csmi_node_type_t_strs [] = {"no type","management","service","login","workload-manager","launch","compute","utility","aggregator",""};

const char* csmi_node_state_t_strs [] = {"undefined","DISCOVERED","IN_SERVICE","OUT_OF_SERVICE","ADMIN_RESERVED","SOFT_FAILURE","MAINTENANCE","CSM_DATABASE_NULL","HARD_FAILURE",""};

const char* csmi_ras_severity_t_strs [] = {"undefined","INFO","WARNING","FATAL",""};

#define STRUCT_DEF "csmi/include/csm_types/struct_defs/common/csm_node_error.def"
#define STRUCT_SUM 0xfb18e213
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



#define STRUCT_DEF "csmi/include/csm_types/struct_defs/common/csmi_err.def"
#define STRUCT_SUM 0x1548afbf
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



