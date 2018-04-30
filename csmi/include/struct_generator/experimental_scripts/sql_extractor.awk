#================================================================================
#
#    csmi/include/struct_generator/sql_extractor.awk
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

#!/bin/awk -f
# encoding: utf-8

# Author: John Dunham (jdunham@us.ibm.com)
# Parses a *.sql file and generates the struct markup for it.

function init_member(table_name, name, type){
    count=tables[current_table]["member_count"]
    
    # Cache the name.
    tables[current_table]["member"][count]["name"]=name

    # Set the comment to null.
    tables[current_table]["member"][count]["comment"]=""

    # Coerce the type to C type.
    sub(/,/,"",type)
    tables[current_table]["member"][count]["type"]=types[type]
    tables[current_table]["member"][count]["db_type"]=type

    # Set the flag for whether the member should be settable in the generated general purpose query builder.
    if ( no_change_types[type] != "" )
    {
        tables[current_table]["member"][count]["no_change"] = 1;
        tables[current_table]["member"][count]["default"] = no_change_types[type];
    }
    else
    {
        tables[current_table]["member"][count]["no_change"] = 0;
        tables[current_table]["member"][count]["default"] = default_values[type];
    }
    
    # Update the counts.
    members[name]=count
    tables[current_table]["member_count"]=count+1

}

function init_table_types(table_name){
    tables[table_num]["member_count"]=0
    tables[table_num]["comment"]=""

    table_names[table_num++]=table_name
}

BEGIN{
    types["bigint"]="int64_t"
    types["bigserial"]="int64_t"
    types["int"]="int32_t"
    types["double"]="float8"
    types["boolean"]="char"
    types["char(1)"]="char"
    types["char(8)"]=types["char(16)"]=types["char(32)"]="char*"
    types["text"]="char*"
    types["timestamp"]="char*"
    types["decimal(54)"]="void" # Don't know the datatype.

    # A collection of types shouldn't be set in the automatic insert all.
    no_change_types["bigserial"]="default"
    no_change_types["timestamp"]="now"

    # Default values for inserts.
    default_values["int64_t"]=""
    default_values["int32_t"]=""
    default_values["float8"]=""
    default_values["char"]=""
    default_values["char*"]=""

    DEFAULT_INDEX=0
    NO_CHANGE_INDEX=1
    DB_TYPE_INDEX=2
    TYPE_INDEX=3
    indicies[DEFAULT_INDEX]="default"
    indicies[NO_CHANGE_INDEX]="no_change" 
    indicies[DB_TYPE_INDEX]="db_type"
    indicies[TYPE_INDEX]="type"
    index_count=TYPE_INDEX+1 


    table_num=1
    current_table=1
}

# Ignore comments.
/^\s*--/ { next }

/CREATE TABLE /{
    current_table=table_num
    init_table_types($3)
    next
}

/^\s*COMMENT ON TABLE/{
    tables[current_table]["comment"]=$2
    next
}

/^\s*COMMENT ON COLUMN/{
    split($4,a,".")
    split($0,b,"'")
    mi=members[a[2]]
    tables[current_table]["member"][mi]["comment"] = b[2]
    next
}

# Ignore all other kinds of comments.
/COMMENT/{ next } 
/CREATE/{ next }
/\s*on\s+[^;]*;/{ next }
/^\s*$/{next}

# TODO
/^\s*PRIMARY/{ next }
/^\s*FOREIGN/{ next }

/^\s*[a-zA-Z_]*\s*[a-zA-Z_]*\s+[^,]+/{
    init_member(current_table,$1,$2)
}


END{
    print ("{ \"table_details\": [ ")
    for( i=0; i < index_count; i++){
        print ("\"" indicies[i] "\"")
        if (i < index_count -1)
            print(",")
    }

    print ("],\"tables\" : {")
    for( i=1; i < table_num; i++){ 
        #print ("-------------"table_names[i]"-------------")
        #print ( "#define TABLE_NAME "table_names[i] )
        
        print ( "\""table_names[i]"\":{" )
#        print ( "\t\"members\":[" )
#
#        for( j=1; j < tables[i]["member_count"]; j++){
#            print("\""tables[i]["member"][j]["name"]"\"") 
#            if (j < tables[i]["member_count"] -1)
#                print (",");
#        }
#        print("],")
        for( j=1; j < tables[i]["member_count"]; j++){
            print ( "\t\""tables[i]["member"][j]["name"]"\" : [" )
            print ( "\""tables[i]["member"][j]["type"]"\"," ) 
            print ( "\""tables[i]["member"][j]["db_type"]"\"," ) 
            print ( "\""tables[i]["member"][j]["no_change"]"\"," ) 
            print ( "\""tables[i]["member"][j]["default"]"\"]" ) 
                
            if (j < tables[i]["member_count"] -1)
                print (",");
        }

        if ( i < table_num -1 )
            print ("},");
        else
            print ("}");
    }
    print("}}")
}
