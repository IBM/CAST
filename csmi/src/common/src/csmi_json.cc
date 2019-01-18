/*================================================================================

    csmi/src/common/src/csmi_json.cc

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/src/common/include/csmi_json.h"
#include "csmi/src/common/include/csmi_common_generated.h"
#include <string>

std::string escapeString(char* c)
{
    std::string output="";
    for( int idx=0; c[idx]; idx++)
    {
        switch (c[idx]) {
            case '"': output.append("\\\""); break;
            case '\\': output.append("\\\\"); break;
            case '\b': output.append("\\b"); break;
            case '\f': output.append("\\f"); break;
            case '\n': output.append("\\n"); break;
            case '\r': output.append("\\r"); break;
            case '\t': output.append("\\t"); break;
            default:
                //if ('\x00' <= *c && *c <= '\x1f') {
                //    output.append("\\u");
                //      << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                //} else {
                output += c[idx];
                //}
        }
    }
    return output;
}

void csmiGenerateJSON(
    std::string& json,
    const char* format,
    void* target,
    const csmi_struct_mapping_t* mapping )
{
    json.append("{");
    
    char* token_str = strdup(format); // Duplicate so the string can be modified.
    char* start_token = token_str;        // The start of the current token.
    char* end_token = token_str;          // The current seek mark for the "tokenizer

    const csmi_struct_node_t* node;         // The struct metadata object.
    const csmi_struct_mapping_t* sub_map;   // A sub mapping for contained structs.
    size_t array_size = 0;                  // The size of an array.

    int offset = 0;
    int type = 0;
    void* member = NULL;

    while ( *end_token != 0 )
    {
        switch (*end_token)
        {
        case '}':
        case ',':
            *end_token = 0;
            node = csmi_search( start_token, mapping );
            start_token = end_token + 1;

            if ( ! node ) break;
            type = node->type >> CSM_TYPE_SHIFT;
            
            determine_array_size(array_size, node->type, node->size_offset, target);
            json.append("\"").append(node->name).append("\":");
        
            switch ( type )
            {
            case CSM_STRING_TYPE:
            {
                if ( node->type & CSM_ARRAY_BIT )
                {
                    json.append("[");
                    char*** array = (char***)((char*)target + node->offset);

                    if ( *array )
                    {
                        for (size_t i=0; i < array_size; ++i)
                        {
                            json.append("\"").append(
                                escapeString((*array)[i])).append("\",");
                        }

                    }
                    if ( array_size > 0 ) json.pop_back();
                    json.append("],");
                }
                else
                {
                    char** str = (char**)((char*)target + node->offset);
                    if ( *str ) 
                        json.append("\"").append(escapeString(*str)).append("\",");
                    else
                        json.append("\"\",");
                }
                break;
            }
            case CSM_ENUM_TYPE:
            {
                size_t idx = (size_t) *((char*)target + node->offset);
                if( idx < node->size_offset )
                    json.append("\"").append(((char**)node->fields)[idx]).append("\",");
                else
                    json.append("\"N/A\",");
                break;
            }
            case CSM_ENUM_BIT_TYPE:
            {
                json.append(std::to_string((size_t)*((char*)target + node->offset)));
                break;
            }

            CSM_PRIMATIVE(json, ((char*)target + node->offset))

            default:
                break;
            }

            break;
        case '{':
            *end_token  = 0; // Null out the end token contents for searching.
            node        = csmi_search( start_token, mapping ); // Retrieve the node.
            start_token = end_token + 1;                       // Update the start of the string.
            offset      = seek_struct_end(start_token);        // Compute the offset.

            if ( ! (node  && (sub_map = (csmi_struct_mapping_t*)node->fields ) ))
            {
                start_token = end_token = end_token + offset;
                break;
            }
            determine_array_size(array_size, node->type, node->size_offset, target);
            
            json.append("\"").append(node->name).append("\":");
            if( array_size > 1 ) json.append("[");

            for( size_t i=0; i < array_size; ++i )
            {

                // Something is broken here, it looks like C++ handles this differently.
                if ( ( member = sub_map->ptr_funct( 
                        ((char*)target + node->offset),(node->type & CSM_ARRAY_BIT),i) ) )
                {
                    csmiGenerateJSON(
                        json, 
                        start_token, 
                        member, 
                        (const csmi_struct_mapping_t*)(node->fields));
                    json.append(",");
                }
            }
            json.pop_back();
            if( array_size > 1 ) json.append("],");
            start_token = end_token = (end_token + offset);

            break;
        default:
            break;
        }
        end_token++;
    }

    if (json.size() > 1 ) json.pop_back();
    json.append("} ");
    free(token_str);
}



