/*================================================================================

    csmi/src/common/include/csm_serialization_x_macros.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_SERIAL_X_MACROS_DEF_
#define CSM_SERIAL_X_MACROS_DEF_
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

// ==============================================================
// Constants
// ==============================================================

// Define some obivious constants just to be sure.
#define CHAR_SIZE sizeof(char)
#define PTR_SIZE  sizeof(void*)
#define UINT64_T_SIZE sizeof(uint64_t)
#define UINT32_T_SIZE sizeof(uint32_t)
#define UINT8_T_SIZE sizeof(uint8_t)

// ==============================================================
// Function Prototypes
// ==============================================================

// Used in concatination of function names using macros.
#define CSM_FUNCT_CAT(A, B) CSM_FUNCT_CAT_(A,B)
#define CSM_FUNCT_CAT_(A, B) A##B

// Defines the len_* function prototype generator.
#define LEN_FUNCT(STRUCT_NAME)              \
    uint32_t CSM_FUNCT_CAT(len_,STRUCT_NAME)(   \
        STRUCT_NAME *target,                \
        uint64_t version_id)

// Defines the serialize_* function prototype generator.
#define PACK_FUNCT(STRUCT_NAME)             \
    void CSM_FUNCT_CAT(pack_,STRUCT_NAME)(      \
        STRUCT_NAME  *target,               \
        char         *buffer,               \
        uint32_t     *offset,               \
        uint32_t      len   ,               \
        uint64_t      version_id )          \

// Defines the serialize_* function prototype generator.
#define SERIALIZE_FUNCT(STRUCT_NAME)        \
    int CSM_FUNCT_CAT(serialize_,STRUCT_NAME)(  \
        STRUCT_NAME  *target,               \
        char        **buf   ,               \
        uint32_t     *buffer_len)

// Defines the unpack_* function prototype generator.
#define UNPACK_FUNCT(STRUCT_NAME)           \
    void CSM_FUNCT_CAT(unpack_,STRUCT_NAME)(    \
        STRUCT_NAME **dest,                 \
        const char   *buffer,               \
        uint32_t     *offset,               \
        uint32_t      len   ,               \
        uint64_t      version_id )               

// Defines the deserialize_* function prototype generator.
#define DESERIALIZE_FUNCT(STRUCT_NAME)        \
    int CSM_FUNCT_CAT(deserialize_,STRUCT_NAME)( \
        STRUCT_NAME **dest,                   \
        const char   *buffer,                 \
        uint32_t      buffer_len)

#define FREE_FUNCT(STRUCT_NAME)             \
    void CSM_FUNCT_CAT(free_,STRUCT_NAME)(      \
        STRUCT_NAME  *target )

#define INIT_FUNCT(STRUCT_NAME)             \
        void CSM_FUNCT_CAT(init_,STRUCT_NAME)(      \
            STRUCT_NAME  *target )


    // ==============================================================
    // Length Serial Macros - Invoked in LEN_FUNCT(STRUCT_NAME)
    // ==============================================================

    /** Helper macro, gets the length of a string, returns 1 if no string is present. */
#define STR_LENGTH(str) ( (str) ? strlen(str) : 0 ) + 1

    /** Length Macros: Assmues a field "len" exists in the invoking code block. */
#define LEN_BASIC(type, name, length_member, metadata)          \
        len += sizeof(type);

#define LEN_STRING(type, name, length_member, metadata)         \
        len += UINT32_T_SIZE + STR_LENGTH(target->name);            

    // For fixed length strings the length matches the supplied value.
#define LEN_STRING_FIXED(type, name, length, metadata) \
        len += length;

#define LEN_ARRAY(type, name, length_member, metadata)          \
        len += sizeof(target->length_member) + (target->name ? \
            sizeof(type) * target->length_member : 0);

    // This is similar to Array, but rather than an indirection the length is the supplied value.
#define LEN_ARRAY_FIXED(type, name, length, metadata) \
        len += sizeof(type) * length;

#define LEN_ARRAY_STR(type, name, length_member, metadata)      \
        len += sizeof(target->length_member);                       \
                                                                    \
        if ( target->name )                                         \
            for( i=0; i < target->length_member; i++)               \
                len += UINT32_T_SIZE + STR_LENGTH(target->name[i]);

#define LEN_ARRAY_STR_FIXED(type, name, length, metadata)       \
        if ( target->name )                                         \
            for ( i=0; i < length; i++ )                            \
                len += UINT32_T_SIZE + STR_LENGTH(target->name[i]); \
        else                                                        \
            len += UINT32_T_SIZE * length

#define LEN_STRUCT(type, name, length_member, metadata)         \
        if( target->name )                                          \
            len += len_##metadata( target->name, version_id ) + UINT32_T_SIZE;  \
        else                                                        \
            len += UINT32_T_SIZE;

#define LEN_ARRAY_STRUCT(type, name, length_member, metadata)         \
        len += sizeof(target->length_member);                             \
                                                                          \
        if( target->name )                                                \
            for ( i=0; i < target->length_member; i++ )                   \
                len += len_##metadata( target->name[i], version_id ) ;               

#define LEN_ARRAY_STRUCT_FIXED(type, name, length, metadata)          \
            if( target->name )                                                \
                for ( i=0; i < length; i++ )                                  \
                    len += len_##metadata( target->name[i],version_id )       \
            else                                                              \
                len += UINT32_T_SIZE * length;

        // This case should literally never happen.
#define LEN_NONE(type, name, length_member, metadata)

        // ==============================================================
        // Packing Serial Macros - Invoked in PACK_FUNCT(STRUCT_NAME)
        // ==============================================================

        // Packing Helpers:
        // --------------------------------------------------------------
        // Packs the element.
#define PACK_ELEMENT(element)                              \
            memcpy(buffer + *offset, &(element), sizeof(element)); \
            *offset += sizeof(element);                          

        // Note the size is in bytes!
#define PACK_ELEMENT_WITH_SIZE(element, size)   \
            memcpy(buffer + *offset, element, size); \
            *offset += size;

        // Adds the length of this element to the buffer.
#define PACK_ELEMENT_SIZE_FIXED(length)                        \
            size_dump = length;                                        \
            memcpy(buffer + *offset, &(size_dump), sizeof(size_dump)); \
            *offset += sizeof(size_dump);                               
        // --------------------------------------------------------------

#define PACK_BASIC(type, name, length_member, metadata)         \
            PACK_ELEMENT(target->name)

#define PACK_STRING(type, name, length_member, metadata)        \
            if ( target->name ){                                        \
                size_dump = strlen(target->name) + 1;                   \
                PACK_ELEMENT(size_dump)                                 \
                PACK_ELEMENT_WITH_SIZE(target->name, size_dump)         \
            }                                                           \
            else {                                                      \
                size_dump = 1;                                          \
                PACK_ELEMENT(size_dump)                                 \
                                                                        \
                size_dump = 0;                                          \
                PACK_ELEMENT_WITH_SIZE(&size_dump, sizeof(char))         \
            }   

        // Always packs the fixed length string.
#define PACK_STRING_FIXED(type, name, length, metadata)  \
            if ( target->name )                                  \
                memcpy( buffer + *offset, &target->name, length);\
            else                                                 \
                memset( buffer + *offset, 0, length );           \
            *offset += length;                               


#define PACK_ARRAY(type, name, length_member, metadata)         \
            size_dump = target->name ? target->length_member : 0;       \
            PACK_ELEMENT(size_dump);                                    \
            for( i=0; i < size_dump; i++ ) {                            \
                PACK_ELEMENT_WITH_SIZE(&target->name[i], sizeof(metadata)) \
            }
            
        // TODO NULL CHECKS?
        // FIXME THIS IS UNTESTED
#define PACK_ARRAY_FIXED(type, name, length, metadata)              \
            for( i=0; i < length; i++ ) {                                   \
                PACK_ELEMENT_WITH_SIZE(&target->name[i], sizeof(metadata))  \
            }

#define PACK_ARRAY_STR(type, name, length_member, metadata)         \
        if ( target->name ){                                            \
            PACK_ELEMENT(target->length_member);                        \
            for( i=0; i < target->length_member; i++ ) {                \
                PACK_STRING(type, name[i], length_member, metadata)     \
            }                                                           \
        }                                                               \
        else {                                                          \
            size_dump=0;                                                \
            PACK_ELEMENT(size_dump);                                    \
        }

#define PACK_ARRAY_STR_FIXED(type, name, length, metadata)   \
        for( i=0; i < length; i++ ) {                            \
            PACK_STRING(type, name[i], length, metadata) \
        }

#define PACK_STRUCT(type, name, length_member, metadata) \
        size_dump = target->name ? 1 : 0;                    \
        PACK_ELEMENT(size_dump)                              \
                                                             \
        pack_##metadata( target->name , buffer, offset, len, version_id );

#define PACK_ARRAY_STRUCT(type, name, length_member, metadata)  \
        size_dump = target->name ? target->length_member : 0;       \
        PACK_ELEMENT(size_dump)                                     \
                                                                    \
        for ( i=0; i < size_dump; i++ ) {                           \
            pack_##metadata( target->name[i], buffer, offset, len, version_id );\
        }

#define PACK_ARRAY_STRUCT_FIXED(type, name, length, metadata)   \
        for ( i=0; i < length; i++ ) {                              \
            pack_##metadata( target->name[i], buffer, offset, len, version_id );\
        }

#define PACK_NONE(type, name, length_member, metadata)

    // ==============================================================
    // Unpacking Serial Macros - Invoked in UNPACK_FUNCT(STRUCT_NAME)
    // ==============================================================

    // Unpacking Helpers:
    // --------------------------------------------------------------
    // Unpacks the element.
#define UNPACK_SIZE()                                                  \
        if ( (*offset + sizeof(size_dump) ) <= len) {                  \
            memcpy( &size_dump, buffer + *offset, sizeof(size_dump) ); \
            *offset += sizeof(size_dump); }\
        else *offset = len+1;

    // --------------------------------------------------------------

#define UNPACK_BASIC(type, name, length_member, metadata)                   \
        if ( (*offset + sizeof(target->name)) <= len) {                     \
         memcpy( &(target->name), buffer + *offset, sizeof(target->name) ); \
         *offset += sizeof(target->name);}
         

#define UNPACK_STRING(type, name, length_member, metadata)   \
        UNPACK_SIZE()                                        \
        if ( (*offset + size_dump) <= len) {                 \
         target->name = (char*) malloc(size_dump);           \
         memcpy( target->name, buffer + *offset, size_dump); \
         *offset += size_dump;}                              \
        else {                                               \
            target->name = NULL;                             \
            *offset = len + 2;                               \
        }

    // FIXME Should this just assume based on the length member?
    // This should be identical to unpack string.
#define UNPACK_STRING_FIXED(type, name, length, metadata) \
        if( (*offset + length) <= len ) {                 \
          memcpy(target->name, buffer + *offset, length); \
          *offset += length; }                            \
        else *offset = len + 2;
                  
#define UNPACK_ARRAY(type, name, length_member, metadata)                   \
        UNPACK_SIZE()                                                       \
        if ( size_dump > 0 && *offset <= len ) {                            \
            target->name = (type) calloc(size_dump, sizeof(metadata));      \
            for ( i=0; i < size_dump; i++){                                 \
             if( (*offset + sizeof(metadata)) <= len ) {                    \
               memcpy(&target->name[i], buffer + *offset, sizeof(metadata));\
               *offset += sizeof(metadata); }                               \
             else *offset = len + 2;                                        \
            }                                                               \
        } else                                                              \
            target->name = NULL;                                               


    // FIXME Do we want to use the predefined array length?
    // TODO Is this right?
#define UNPACK_ARRAY_FIXED(type, name, length, metadata)                     \
        if( (*offset + sizeof(metadata) ) <= len ) {                         \
          memcpy(&target->name, buffer + *offset, length * sizeof(metadata));\
          *offset += length; }                                               \
        else *offset = len + 2;

#define UNPACK_ARRAY_STR_FIXED(type, name, length, metadata)                  \
        if( length > 0 && *offset <= len ){                                   \
            target->name = (type) malloc(PTR_SIZE * size_dump);               \
                                                                              \
            for ( i=0; i < length; i++) {                                     \
                if( (*offset + sizeof(str_offset)) <= len ) {                 \
                  memcpy( &str_offset, buffer + *offset, sizeof(str_offset) );\
                  *offset += sizeof(str_offset); }                            \
                                                                              \
                if( (*offset + str_offset) <= len ) {                         \
                  target->name[i] = (char*) malloc(str_offset);               \
                  memcpy(target->name[i], buffer + *offset, str_offset);      \
                  *offset += str_offset; }                                    \
                else {                                                        \
                    target->name[i] = strdup("");                             \
                    *offset += length; }                                      \
            }                                                                 \
        } else                                                                \
            target->name = NULL;                                       


#define UNPACK_ARRAY_STR(type, name, length_member, metadata)               \
        UNPACK_SIZE()                                                       \
        UNPACK_ARRAY_STR_FIXED(type, name, size_dump, metadata);            \

    //XXX Struct PTR vs non PTR?
#define UNPACK_STRUCT(type, name, length_member, metadata)                  \
        UNPACK_SIZE()                                                       \
        if( size_dump > 0 && *offset <= len )                               \
            unpack_##metadata( &(target->name), buffer, offset, len, version_id);  \
        else                                                                \
            target->name = NULL;

    //TODO  Verify
#define UNPACK_ARRAY_STRUCT(type, name, length_member, metadata)             \
        UNPACK_SIZE()                                                        \
        if ( size_dump > 0 && *offset <= len ){                              \
            target->name = ( type ) malloc(sizeof(type) * size_dump);        \
            for ( i=0; i < size_dump; i++){                                  \
                unpack_##metadata( &(target->name[i]), buffer, offset, len, version_id); \
            }                                                                \
        } else                                                               \
            target->name = NULL;


    //TODO 
#define UNPACK_ARRAY_STRUCT_FIXED(type, name, length, metadata) \
        for ( i=0; i < length; i++)                             \
            unpack_##metadata( &(target->name[i]), buffer, offset, len, version_id);


#define UNPACK_NONE(type, name, length_member, metadata)

    // ==============================================================
    // Free Array Serial Macros - Invoked in FREE_FUNCT(STRUCT_NAME)
    // ==============================================================

    // Doesn't need to be deallocated.
#define FREE_ARRAY_BASIC(type, name, length_member, metadata)
#define FREE_ARRAY_STRING(type, name, length_member, metadata)
#define FREE_ARRAY_STRING_FIXED(type, name, length_member, metadata)
#define FREE_ARRAY_ARRAY(type, name, length_member, metadata)   
#define FREE_ARRAY_ARRAY_FIXED(type, name, length_member, metadata)
#define FREE_ARRAY_STRUCT(type, name, length_member, metadata) 
#define FREE_ARRAY_NONE(type, name, length_member, metadata)

    // TODO Test for null?
#define FREE_ARRAY_ARRAY_STR_FIXED(type, name, length, metadata) \
        if (target->name) {                                          \
            for ( i=0; i < length; i++ ) {                           \
                free(target->name[i]);                               \
            }                                                        \
        }

    // Reuse the fixed length macro.
#define FREE_ARRAY_ARRAY_STR(type, name, length_member, metadata) \
        if (target->name) {                                           \
            for ( i=0; i < target->length_member; i++ ) {             \
                free(target->name[i]);                                \
            }                                                         \
            free(target->name);                                       \
        }

    //TODO 
#define FREE_ARRAY_ARRAY_STRUCT_FIXED(type, name, length, metadata) \
        if (target->name) {                                             \
            for ( i=0; i < length; i++ ) {                              \
                free_##metadata(target->name[i]);                       \
                free(target->name[i]);                                  \
            }                                                           \
        }

    //TODO 
#define FREE_ARRAY_ARRAY_STRUCT(type, name, length_member, metadata) \
        if (target->name) {                                              \
            for ( i=0; i < target->length_member; i++ ) {                \
                free_##metadata(target->name[i]);                        \
                free(target->name[i]);                                   \
            }                                                            \
            free(target->name);                                          \
        }

    // ==============================================================
    // Free Serial Macros - Invoked in FREE_FUNCT(STRUCT_NAME)
    // ==============================================================

    // Doesn't need to be deallocated.
#define FREE_BASIC(type, name, length_member, metadata)
#define FREE_STRING_FIXED(type, name, length_member, metadata)
    // TODO verify there are no leaks!
#define FREE_ARRAY_FIXED(type, name, length_member, metadata)
#define FREE_ARRAY_STR(type, name, length_member, metadata) 
#define FREE_ARRAY_STR_FIXED(type, name, length_member, metadata)
#define FREE_ARRAY_STRUCT(type, name, length_member, metadata)
#define FREE_ARRAY_STRUCT_FIXED(type, name, length_member, metadata)
#define FREE_NONE(type, name, length_member, metadata)

#define FREE_STRING(type, name, length_member, metadata)    \
    if(target->name) free(target->name);
    
#define FREE_ARRAY(type, name, length_member, metadata)     \
    if(target->name) free(target->name);

// TODO Should there only be struct pointers?
#define FREE_STRUCT(type, name, length_member, metadata)    \
    if(target->name) { free_##metadata(target->name); free(target->name); } 

// ==============================================================
// Init Serial Macros - Invoked in INIT_FUNCT(STRUCT_NAME)
// ==============================================================

// Members that don't get initialized at the moment.
#define INIT_ARRAY_FIXED(name, init_value, length)
#define INIT_ARRAY_STRUCT_FIXED(name, init_value, length)
#define INIT_ARRAY_STR_FIXED(name, init_value, length)
#define INIT_NONE(name, init_value, length)

#define INIT_STRING_FIXED(name, init_value, length) \
    memset((char*)target->name, '\0', length);

#define INIT_BASIC(name, init_value, length)        \
    target->name = init_value;

#define INIT_STRING(name, init_value, length)       \
    target->name = NULL;

#define INIT_ARRAY(name, init_value, length)        \
    target->name = NULL;

#define INIT_ARRAY_STR(name, init_value, length)    \
    target->name = NULL;

#define INIT_STRUCT(name, init_value, length)        \
    target->name = NULL;

#define INIT_ARRAY_STRUCT(name, init_value, length)  \
    target->name = NULL;

#endif
