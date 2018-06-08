/*================================================================================

    csmi/src/common/include/csmi_struct_hash.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_STRUCT_HASH_H__
#define __CSMI_STRUCT_HASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "csmi_common_generated.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#define CSM_STRUCT_MAP_( map, struct_name ) map##struct_name
/** @brief Gets the constant struct map name. */
#define CSM_STRUCT_MAP( struct_name ) &CSM_STRUCT_MAP_(map_,struct_name)

/** @brief Wraps a format string for apis with wrapper structs.
 * Callocs a char* equal to the size of 3 strings combined.
 *
 * @param[out] str_ptr A null char* for storing the constructed format string.
 * @param[in]  format_start A null terminated start of a format string.
 * @param[in]  format_custom A null terminated custom format string.
 * @param[in]  format_end A null terminated end to a format string.
 */
#define CSM_WRAP_FORMAT_STRING( str_ptr, format_start, format_custom, format_end )                       \
   str_ptr = malloc(strlen(format_start) + strlen(format_custom) + strlen(format_end) + 1);\
   strcpy(str_ptr, format_start);                                                                        \
   strcat(str_ptr, format_custom);                                                                       \
   strcat(str_ptr, format_end);

// Determines the size of the supplied field's array.
#define determine_array_size(array_size, node_type, node_size, struct_ptr)                   \
    array_size = node_type & CSM_ARRAY_BIT ?                                                 \
        ( node_type & CSM_FIXED_BIT ? node_size : (size_t) *((char*)struct_ptr + node_size)) \
        : 1

/// Represents the array bit in the type field of the @ref csmi_struct_tree_t.
#define CSM_ARRAY_BIT 1 

/// Represents the fixed bit in the type field of the @ref csmi_struct_tree_t.
#define CSM_FIXED_BIT 2

/// Represents the bit shift to reach the type of the field.
#define CSM_TYPE_SHIFT 2

extern const int csm_min_printable_type; ///< The minimum value in the array representing a printable type (e.g. int ).
extern const int csm_type_formatter_len; ///< The length of the type formatter array.
extern const char* csm_type_formatter[]; ///< Defines formatters for printf and struct member types.

struct csmi_struct_mapping_t; // Forward declaration for struct mapping.

/**
 * @brief A node in a BST for resolving strings into struct fields.
 */
typedef struct {
    const char* name;              ///< The name of the field (if this is a struct, this is the struct name).
    size_t offset;                 ///< The offset of the field from the start of the struct.
    size_t size_offset;            ///< The offset of the field containing sizing data (for arrays), if fixed field is set this is the length.
//    const struct csmi_struct_mapping_t* fields; ///< A map of subfields, set to NULL if the field is terminal.
    const void* fields;            ///< A map of subfields, set to NULL if the field is terminal.
    uint32_t hash_val;             ///< The hashed value of the field name (for testing collisions).
    char type;                     ///< A bit string representing the type. The first and second bits represent whether the type is fixed or an array. The third on is a numeric representation of the type (representing an index in @ref csm_type_formatter).
} csmi_struct_node_t;


typedef void* (*csmi_offset_funct)(void*,size_t);

typedef void* (*csmi_malloc_funct)(size_t);

/**
 * @brief A struct for resolving strings into struct fields.
 */
typedef struct csmi_struct_mapping_t{
    size_t num_fields;                     ///< The number fields in the tree.
    const csmi_struct_node_t* field_tree;  ///< A array based tree, this tree is lean (e.g. null leaf nodes are culled.).
    csmi_offset_funct ptr_funct;
    //csmi_malloc_funct malloc_funct;
}csmi_struct_mapping_t;


/** @brief The enumerated type for csm printing.
 */
typedef enum {
    CSM_YAML = 0,
    CSM_JSON,
    CSM_CSV
} csmi_print_type;

// ================================================================================================

/** @brief Implementation of the djb2 hashing algorithm for name resolution.
 * @note This algorithm may have collisions, but since this is a compile time binding, collisions should only occur in the failure path.
 *
 * @param[in] str A c-string to compute the hash value of.
 * @return The 32-bit hashed value of the @p str parameter.
 */
static inline uint32_t csmi_djb2(char* str){
    uint32_t hash = 5381;
    int c;
    while ( (c = *str++) ){ hash = ((hash << 5) + hash) + c; }
    return hash;
}

/**
 * @brief Performs a search on the supplied @ref csmi_struct_mapping_t tree for the the supplied @p string.
 *
 * @param[in] string The string to perform the search operation on.
 * @param[in] tree   The tree to search.
 *
 * @return The node matching @p string, if the string was not found return null.
 */
static inline const csmi_struct_node_t* csmi_search(
    char* string, const csmi_struct_mapping_t* tree)
{
    size_t index = 0;
    size_t target_index = tree->num_fields;
    uint32_t hash_val = csmi_djb2 ( string );

    while (index < tree->num_fields)
    {
        // If the hash value is greater, search to the right side of the tree.
        // Else if the hash value is less, search the left side of the tree.
        // Else the node has been found.
        if ( hash_val > tree->field_tree[index].hash_val )
        {
            index = 2 * index + 2;
        }
        else if ( hash_val < tree->field_tree[index].hash_val )
        {
            index = 2 * index + 1;
        }
        else
        {
            target_index = index;
            index        = tree->num_fields;
        }
    }
    
    // If the target index was valid and the string matches return the tree.
    if ( target_index < tree->num_fields && 
            strcmp(tree->field_tree[target_index].name,string) == 0 )
    {
        return &tree->field_tree[target_index];
    }
    else
        return NULL;
}

#define csmi_printer(print_type, format_str, print_struct, mapping) \
    csmi_printer_internal( print_type, format_str, print_struct, mapping, 0)

/**
 * @brief Seeks the end of the supplied print string.
 *
 * INTERNAL USE ONLY.
 *
 * @param[in] print_string A pointer to an arbitrary print string.
 *
 * @return The number of characters until the end of the struct in the print string.
 */
int seek_struct_end( char* print_string );

/** @todo document. */
void csmi_printer_internal( 
    csmi_print_type print_type,
    char* format_str, 
    void* print_struct, 
    const csmi_struct_mapping_t* mapping,
    uint8_t depth );


#ifdef __cplusplus
}
#endif

#endif
