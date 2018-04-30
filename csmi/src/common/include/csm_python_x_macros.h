/*================================================================================

    csmi/src/common/include/csm_python_x_macros.h

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_PYTHON_X_MACROS_DEF_
#define CSM_PYTHON_X_MACROS_DEF_
#include <string.h>

#define CSM_GEN_DOCSTRING(msg, tuple_doc) msg "\ntuple: \n\t(<return code>,<handler id>" tuple_doc ")"

// Getters and Setters.
#define CSM_CSTR_GET(TYPE,FIELD) +[](const TYPE& t) { return t.FIELD ? (const char*)t.FIELD : "";}
#define CSM_CSTR_SET(TYPE,FIELD) +[](TYPE& t, char* const text){ if ( t.FIELD ) free(t.FIELD);\
        t.FIELD = strdup(text); }

#define CSM_CSTR_FIXED_GET(TYPE,FIELD,LEN) +[](const TYPE& t) { return t.FIELD ? (const char*)t.FIELD : ""; }
#define CSM_CSTR_FIXED_SET(TYPE,FIELD,LEN) +[](TYPE& t, char* const text){  strncpy(text, t.FIELD, LEN ); }

#define CSM_ARRAY_GET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](const TYPE& e, size_t i){\
    return e.FIELD && i < (size_t)e.SIZE ? (const char*)e.FIELD[i] : 0 ;}, args("e","i"), "Array getter for "#FIELD
#define CSM_ARRAY_SET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](const TYPE& e ){;},args("e"), "Array setter for "#FIELD

#define CSM_ARRAY_FIXED_GET(TYPE,FIELD,FIELD_TYPE,SIZE,DEFAULT) NULL
#define CSM_ARRAY_FIXED_SET(TYPE,FIELD,FIELD_TYPE,SIZE,DEFAULT) NULL 

#define CSM_ARRAY_STR_GET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](const TYPE& e, size_t i){\
    return e.FIELD && i < (size_t)e.SIZE ? (const char*)e.FIELD[i] : "";}, args("e","i"), "Array getter for "#FIELD
#define CSM_ARRAY_STR_SET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](TYPE& e, boost::python::list& l){\
    if(e.FIELD) { for(size_t i=0 ; i < (size_t)e.SIZE; ++i) {free(e.FIELD[i]);} free(e.FIELD); }\
        e.SIZE=len(l); e.FIELD = (char**) malloc(sizeof(char*) * e.SIZE); \
        for(size_t i=0 ; i < (size_t)e.SIZE; ++i) { std::string temp = boost::python::extract<std::string>(l[i]);\
            e.FIELD[i] = strdup(temp.c_str()); }\
        },args("e"), "Array setter for "#FIELD

#define CSM_ARRAY_STR_FIXED_GET(TYPE,FIELD,FIELD_TYPE,SIZE) NULL
#define CSM_ARRAY_STR_FIXED_SET(TYPE,FIELD,FIELD_TYPE,SIZE) NULL 

#define CSM_ARRAY_STRUCT_GET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](const TYPE& e, size_t i){\
    return e.FIELD && i < (size_t)e.SIZE ? *e.FIELD[i] : FIELD_TYPE();}, args("e","i"), "Array getter for "#FIELD
#define CSM_ARRAY_STRUCT_SET(TYPE,FIELD,FIELD_TYPE,SIZE) NULL 

#define CSM_ARRAY_STRUCT_FIXED_GET(TYPE,FIELD,FIELD_TYPE,SIZE) +[](const TYPE& e, size_t i){\
    return e.FIELD && i < (size_t)e.SIZE ? *e.FIELD[i] : FIELD_TYPE();}, args("e","i"), "Array getter for "#FIELD
#define CSM_ARRAY_STRUCT_FIXED_SET(TYPE,FIELD,FIELD_TYPE,SIZE) NULL 

// =====================================================================================
#define CSM_PASTER(a, b)  a  ## :: ## b
#define CSM_EVALUATOR(a,b) CSM_PASTER(a, b)

#define BASIC_PROPERTY(class_name, type, name, length, init_value, metadata) \
    .add_property(#name, &CSM_EVALUATOR(class_name, name), &CSM_EVALUATOR(class_name, name))

#define STRING_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .add_property(#name, CSM_CSTR_GET(class_name, name), CSM_CSTR_SET(class_name, name))

#define STRING_FIXED_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .add_property(#name, CSM_CSTR_FIXED_GET(class_name, name), CSM_CSTR_FIXED_SET(class_name, name))

#define ARRAY_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .def("get_"#name, CSM_ARRAY_GET(class_name, name, metadata, length))
//#define ARRAY_PROPERTY(class_name, type, name, length, metadata)
//    .def("set_"#name, CSM_ARRAY_SET(class_name, name, metadata, length))

#define ARRAY_FIXED_PROPERTY(class_name, type, name, length, init_value, metadata)
//#define ARRAY_FIXED_PROPERTY(class_name, type, name, length, metadata)
//    .def("get_"#name, CSM_ARRAY_FIXED_GET(class_name, name, metadata, length))
//    .def("set_"#name, CSM_ARRAY_FIXED_SET(class_name, name, metadata, length))

#define ARRAY_STR_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .def("get_"#name, CSM_ARRAY_STR_GET(class_name, name, metadata,  length))\
    .def("set_"#name, CSM_ARRAY_STR_SET(class_name, name, metadata, length))
//#define ARRAY_STR_PROPERTY(class_name, type, name, length, metadata)
//    .def("set_"#name, CSM_ARRAY_STR_SET(class_name, name, metadata, length))

#define ARRAY_STR_FIXED_PROPERTY(class_name, type, name, length, init_value, metadata)
//#define ARRAY_STR_FIXED_PROPERTY(class_name, type, name, length, metadata)
//    .def("get_"#name, CSM_ARRAY_STR_FIXED_GET(class_name, name, metadata, length))
//    .def("set_"#name, CSM_ARRAY_STR_FIXED_SET(class_name, name, metadata, length))

#define STRUCT_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .add_property(#name, make_getter(metadata, return_value_policy<reference_existing_object>()),\
        make_setter(metadata, return_value_policy<reference_existing_object>()))

#define ARRAY_STRUCT_PROPERTY(class_name, type, name, length, init_value, metadata)\
    .def("get_"#name, CSM_ARRAY_STRUCT_GET(class_name, name, metadata, length))\

//    .def("set_"#name, CSM_ARRAY_STRUCT_SET(class_name, name, metadata, length))

#define ARRAY_STRUCT_FIXED_PROPERTY(class_name, type, name, length, init_value, metadata)
//#define ARRAY_STRUCT_FIXED_PROPERTY(class_name, type, name, length, metadata)
//    .def("get_"#name, CSM_ARRAY_STRUCT_FIXED_GET(class_name, name, metadata, length))
//    .def("set_"#name, CSM_ARRAY_STRUCT_FIXED_SET(class_name, name, metadata, length))

#endif

