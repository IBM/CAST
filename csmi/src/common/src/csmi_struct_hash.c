/*================================================================================

    csmi/src/common/src/csmi_struct_hash.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/src/common/include/csmi_struct_hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CSM_MAX_DEPTH 10

#define CSMI_WILDCARD '%'


// Determines the size of the supplied field's array.
#define determine_array_size(array_size, node_type, node_size, struct_ptr)                   \
    array_size = node_type & CSM_ARRAY_BIT ?                                                 \
        ( node_type & CSM_FIXED_BIT ? node_size : (size_t) *((char*)struct_ptr + node_size)) \
        : 1

/**
 * @brief Seeks the end of the supplied print string.
 *
 * INTERNAL USE ONLY.
 *
 * @param[in] print_string A pointer to an arbitrary print string.
 *
 * @return The number of characters until the end of the struct in the print string.
 */
int seek_struct_end( char* print_string ) 
{
    char* start = print_string;
    int structs = 1;
    while (structs > 0 && *print_string) {
        switch (*print_string)
        {
            case '{': structs++; break;
            case '}': structs--; break;
        }
        print_string++;
    }
   
    return print_string - start;
}

// Internal only?
void csmi_print_all( 
    uint8_t depth,
    csmi_print_type print_type,
    void* print_struct,
    const csmi_struct_mapping_t* mapping )
{
    if ( !print_struct )
    {
        printf("\n");
        return;
    }

    size_t index = 0;
    size_t i = 0;
    size_t array_size = 0;
    int type = 0;
    csmi_struct_node_t node;
    const csmi_struct_mapping_t* sub_map;


    // Number of spaces for this level.
    int num_spaces = depth*2;
    int child_offset = depth > 0 ? 2 : 0;
    int is_not_first = 0;
    
    #define get_spaces() num_spaces * is_not_first + child_offset  * is_not_first
    
    for (; index < mapping->num_fields; index++ )
    {
        node = mapping->field_tree[index]; 
        
        if ( !node.name )
        {
            continue;
        }

        determine_array_size(array_size, node.type, node.size_offset, print_struct);

        type = node.type >> CSM_TYPE_SHIFT;
       
        printf("%*c%s: ", get_spaces(), ' ', node.name);
        switch ( type )
        {
            case CSM_STRING_TYPE:
                
                if ( node.type & CSM_ARRAY_BIT )
                {
                    printf("\n");
                    for ( i=0; i< array_size; ++i)
                    {
                        printf("%*c- %s\n",  (depth+2)*2,' ',
                            (*(char***)(print_struct + node.offset))[i]);
                    }
                }
                else
                {
                    printf("%s\n", *(char**)(print_struct + node.offset));
                }
                break;

            case CSM_ENUM_TYPE:
            {
                size_t idx = (size_t) *((char*)print_struct + node.offset);
                if( idx < node.size_offset )
                    printf("%s\n",((char**)node.fields)[idx]);
                else
                    printf("N/A\n");
                break;
            }
            case CSM_ENUM_BIT_TYPE:
                printf("%d\n",
                    *((char*)print_struct + node.offset));
                break;

            case CSM_MISC_TYPE:
                sub_map = (csmi_struct_mapping_t*)node.fields;

                
                if ( sub_map )
                {
                    for( i=0; i < array_size; ++i )
                    {
                        if ( array_size > 1 )
                            printf("\n\n%*c -", get_spaces(), ' '); 
                        else
                            printf("\n%*c  ", get_spaces(), ' ');
                        
                        csmi_print_all(
                            depth+1,     
                            print_type,
                            sub_map->ptr_funct((print_struct + node.offset), i),
                            sub_map);
                    }
                }
                break;

            //CSM_PRINT_PRIMATIVE( (print_struct + node->offset), "\n");
            CSM_PRINT_PRIMATIVE( (print_struct + node.offset), "\n")

            default:
                printf("\n");
                break;
        }
        is_not_first=1;
    }
}

typedef enum {
    CSMI_SCANNING,
    CSMI_GATHER,
    CSMI_PRINT_ALL
} csmi_parse_states;

void csmi_printer_internal(
    csmi_print_type print_type,     
    char* format_str,
    void* print_struct,
    const csmi_struct_mapping_t* mapping,
    uint8_t depth )
{
    // EARLY RETURN
    // If the struct was missing return the computed offset.
    if ( !print_struct ) return;

    // Track the printer state
    csmi_parse_states parse_state = CSMI_SCANNING;

    // Set up the token pointers.
    char* token_str = strdup(format_str); // Duplicate so the string can be modified.
    char* start_token = token_str;        // The start of the current token.
    char* end_token = token_str;          // The current seek mark for the "tokenizer"

    const csmi_struct_node_t* node;         // The struct metadata object.
    const csmi_struct_mapping_t* sub_map;   // A sub mapping for contained structs.
    size_t array_size = 0;                  // The size of an array.

    size_t array_index = 0;
    int type = 0;
    int offset = 0;
    void* member = NULL;

    // Number of spaces for this level.
    int num_spaces = depth*2;
    int child_offset = depth > 0 ? 2 : 0;
    int is_not_first = 0;
    #define get_spaces() num_spaces * is_not_first + child_offset  * is_not_first

    // Iterate over the string
    while ( *end_token != 0 )
    {
        switch (*end_token)
        {
        case CSMI_WILDCARD:
            csmi_print_all( depth, print_type, print_struct, mapping );
            break;
        // Process print dividers.
        case '}':
        case ',':
            *end_token = 0;
            node = csmi_search( start_token, mapping );
            start_token = end_token + 1;
            
            // If the string couldn't be found ignore the field.
            if ( ! node ) 
            {
                break;
            }

            type = node->type >> CSM_TYPE_SHIFT;

            determine_array_size(array_size, node->type, node->size_offset, print_struct);

            printf("%*c%s: ", get_spaces(), ' ', node->name);
            switch ( type )
            {
            case CSM_STRING_TYPE:
                if ( node->type & CSM_ARRAY_BIT )
                {
                    printf("\n");
                    for ( array_index=0; array_index< array_size; ++array_index)
                    {
                        printf("%*c- %s\n",  (depth+2)*2,' ',
                            (*(char***)(print_struct + node->offset))[array_index]);
                    }
                }
                else printf("%s\n",  *(char**)(print_struct + node->offset));

                break;
            case CSM_ENUM_TYPE:
            {
                size_t idx = (size_t) *((char*)print_struct + node->offset);
                if( idx < node->size_offset )
                    printf("%s\n",((char**)node->fields)[idx]);
                else
                    printf("N/A\n");
                break;
            }
            case CSM_ENUM_BIT_TYPE:
                printf("%d\n",*((char*)print_struct + node->offset));
                break;

            // Print primative types.
            CSM_PRINT_PRIMATIVE( (print_struct + node->offset), "\n");

            default:
                printf("\n");
                break;
            }

            is_not_first = 1;
            break;
        case '{':
            *end_token  = 0; // Null out the end token contents for searching.
            node        = csmi_search( start_token, mapping ); // Retrieve the node.
            start_token = end_token + 1;                       // Update the start of the string.
            offset      = seek_struct_end(start_token);        // Compute the offset.
            
            // Verify that the node is found and that it has a sub map.
            if ( ! (node  && (sub_map = (csmi_struct_mapping_t*)node->fields ) ))
            {
                start_token = end_token = end_token + offset;
                break;
            }

            // Compute the size of the struct array.
            determine_array_size(array_size, node->type, node->size_offset, print_struct);
            
            // TODO remove the YAML version of this code.
            printf("%*c%s: \n", get_spaces(), ' ', node->name);
            for( array_index=0; array_index < array_size; ++array_index )
            {
                if ( ( member = sub_map->ptr_funct((print_struct + node->offset), array_index) ) )
                {
                    if ( array_size > 1 )
                        printf("\n%*c -", get_spaces(), ' '); 
                    else
                        printf("%*c  ", get_spaces(), ' ');

                    csmi_printer_internal(
                        print_type,
                        start_token,
                        member,
                        (node->fields),
                        depth+1);
                }
            }
            
            start_token = end_token = (end_token + offset);

            is_not_first = 1;
            break;

        default:
            switch(parse_state)
            {
                case CSMI_SCANNING:
                    ;
                    break;
                default:
                ;
            }

        }
        end_token++;
    }

    free(token_str);
}

