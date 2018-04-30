/*================================================================================

    csmi/include/csm_api_macros.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_SERIAL_MACROS_DEF_
#define CSM_SERIAL_MACROS_DEF_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/** @file csm_api_macros.h
 * @brief A collection of macros for manipulation of the CSM APIs.
 * This file should not be directly included, please include @ref csm_api_common.h instead.
 */

//==============================================================
// Boolean helpers
//==============================================================

/** @ingroup common_apis
 * @def csm_convert_psql_bool(character)
 * Checks an incoming character for truth in the psql model.
 * @param[in] character The character to check for truth.
 * @return True if the database value was true.
 */
#define csm_convert_psql_bool(character) character == 't' || character == 'T' || character == '0'

/** @ingroup common_apis
 * @def csm_init_lib()
 * Prints a boolean character (syntaxtic sugar for a ternary operator).
 * @param[in] bool_val The boolean value to print out.
 * @param[in] t The true character/string.
 * @param[in] f The false character/string.
 */
#define csm_print_bool_custom(bool_val,t,f) bool_val ? t : f

/** @ingroup common_apis
 * @def csm_init_lib()
 * Prints `t` or `f` depending on the truth of @p bool_val.
 * @param[in] bool_val A boolean of type @ref csm_bool.
 */
#define csm_print_bool(bool_val) csm_print_bool_custom(bool_val,'t','f')

//==============================================================
// Commandline parsers
//==============================================================
/** @brief Converts a c-string to an int32_t and reports if it could not be parsed.
 * 
 * @note Performs an strol on @p str.
 *
 * @note @p usage should also handle cleaning up the struct. 
 * @note @p usage is invoked as follows: usage();
 *
 * @param[out] destination An integer to hold the string to an integer.
 * @param[in]  str         A c-string to convert to an integer.
 * @param[in]  arg_test    A char pointer to validate the string's integer values.
 * @param[in]  context     A string providing more details to the debug message.
 * @param[in]  usage       A function to display usage, typically in the format of:
 *                              csm_free_struct_ptr(STRUCT_TYPE, struct); help
 */
#define csm_str_to_int32( destination, str, arg_test, context, usage )      \
    destination = strtol(str, &arg_test, 10);                           \
    if( *arg_test )                                                     \
    {                                                                   \
        fprintf(stderr,"Invalid value for %s: %s\n",context, str);      \
        usage();                                                        \
                                                                        \
        return CSMERR_INVALID_PARAM;                                    \
    }

/** @brief Converts a c-string to an int64_t and reports if it could not be parsed.
 * 
 * @note Performs an stroll on @p str.
 * @note @p usage should also handle cleaning up the struct.
 *
 * @param[out] destination An integer to hold the string to an integer.
 * @param[in]  str         A c-string to convert to an integer.
 * @param[in]  arg_test    A char pointer to validate the string's integer values.
 * @param[in]  context     A string providing more details to the debug message.
 * p
 * @param[in]  usage       A function to display usage, typically in the format of:
 *                              csm_free_struct_ptr(STRUCT_TYPE, struct); help
 *                          
 */
#define csm_str_to_int64( destination, str, arg_test, context, usage ) \
    destination = strtoll(str, &arg_test, 10);                     \
    if( *arg_test )                                                \
    {                                                              \
        fprintf(stderr,"Invalid value for %s: %s\n",context, str); \
        usage();                                                   \
                                                                   \
        return CSMERR_INVALID_PARAM;                               \
    }

/** @brief Converts a c-string to an double and reports if it could not be parsed.
 * 
 * @note Performs an stroll on @p str.
 * @note @p usage should also handle cleaning up the struct.
 *
 * @param[out] destination A double to hold the results of the string.
 * @param[in]  str         A c-string to convert to an integer.
 * @param[in]  arg_test    A char pointer to validate the string's integer values.
 * @param[in]  context     A string providing more details to the debug message.
 * p
 * @param[in]  usage       A function to display usage, typically in the format of:
 *                              csm_free_struct_ptr(STRUCT_TYPE, struct); help
 *                          
 */
#define csm_str_to_double( destination, str, arg_test, context, usage ) \
    destination = strtod(str, &arg_test);                           \
    if( *arg_test )                                                 \
    {                                                               \
        fprintf(stderr,"Invalid value for %s: %s\n",context, str);  \
        usage();                                                    \
                                                                    \
        return CSMERR_INVALID_PARAM;                                \
    }

/** @brief Duplicates the c-string into @p destination (used by @ref csm_parse_csv).
 *
 * @param[out] destination A char* to store the duplicated string in.
 * @param[in]  str         The c-string to duplicate.
 * @param[in]  arg_test    Unused.
 * @param[in]  context     Unused.
 * @param[in]  usage       Unused.
 */
#define csm_str_to_char( destination, str, arg_test, context, usage )       \
    destination = strdup( str );

/** @brief A helper macro to standardize c-string parsing.
 * 
 * @param[in]  csv_string        A c-string (char*) containig a list of comma separated values.
 * @param[out] array             An array to store the separated list in, assumed to not be 
 *                                  allocated.
 * @param[out] array_size        A variable to store the length of the array in.
 * @param[in]  array_member_type The type of an array member (for char** it would be char*).
 * @param[in]  str_to_funct      The str_to_* macro to invoke, should be one of the following:
 *                                  @ref csm_str_to_char, @ref csm_str_to_int64, @ref csm_str_to_int32
 * @param[out] arg_test          An argument tester for string to int conversion.
 * @param[in]  context           A string to give context to any error messages generated:
 *                                  "Invalid value for **context**:"
 * @param[in]  usage       A function to display usage, typically in the format of:
 *                              csm_free_struct_ptr(STRUCT_TYPE, struct); help
 *
 */
#define csm_parse_csv(   csv_string,    array, array_size, array_member_type,       \
                   str_to_funct, arg_test,    context,             usage)       \
    uint32_t i = 0;                                                             \
    char *save_ptr;                                                             \
    char *arg_str = strdup(csv_string);                                         \
    char *val_str = strtok_r(arg_str, ",", &save_ptr);                          \
                                                                                \
    array_size = 0;                                                             \
    while (val_str != NULL)                                                     \
    {                                                                           \
        array_size++;\
        val_str = strtok_r(NULL, ",", &save_ptr);                           \
    }                                                                           \
    free(arg_str);                                                              \
                                                                                \
    if ( array_size == 0 ) {                                                    \
        csmutil_logging(error, "%s had no usable values.",                      \
            context );                                                          \
        usage();                                                                \
        return CSMERR_INVALID_PARAM;                                            \
    }                                                                           \
                                                                                \
    array  = (array_member_type*)malloc(sizeof(array_member_type) * array_size);\
    val_str = strtok_r(csv_string, ",", &save_ptr);                             \
    while (val_str != NULL)                                                     \
    {                                                                           \
        str_to_funct(array[i], val_str, arg_test, context, usage )              \
        i++;                                                                    \
        val_str = strtok_r(NULL, ",", &save_ptr);                               \
    }


#define CSM_NO_MEMBER  ///< A no member def for @ref csm_parse_csv_to_struct for the member parameter (used for arrays).

/** 
 * @brief Redeclaration of strtol base 10, for use in  @ref csm_parse_csv_to_struct.
 * @param[in] string The string to parse.
 */
#define csm_to_int32( string ) strtol(string, NULL, 10)

/** 
 * @brief Redeclaration of strtoll base 10, for use in  @ref csm_parse_csv_to_struct.
 * @param[in] string The string to parse.
 */
#define csm_to_int64( string ) strtoll(string, NULL, 10)

/** 
 * @brief Redeclaration of strtod, for use in  @ref csm_parse_csv_to_struct.
 * @param[in] string The string to parse.
 */
#define csm_to_double( string ) strtod(string, NULL)

/** 
 * @brief Retrieves the zeroth element of a string, for use in  @ref csm_parse_csv_to_struct.
 * @param[in] string The string to parse.
 */
#define csm_to_char( string ) string[0]

/**
 * @brief Declares the variables necessary to invoke @ref csm_parse_csv_to_struct.
 * Declares i, saveptr and nodeStr.
 */
#define csm_prep_csv_to_struct()\
    uint32_t i;                 \
    char *saveptr;              \
    char *nodeStr;
	
/**
 * @brief Declares the variables necessary to invoke @ref csm_sub_struct_reset_data.
 */
#define csm_prep_sub_struct()\
    char* pEnd;                   \
	int j = 0;                    \
	int      field_length = 0;    \
	char*    temp_trimmed = NULL; \
	char**   temp_strings = NULL; \
	int32_t* temp_32s     = NULL; \
	int64_t* temp_64s     = NULL; \
	double*  temp_doubles = NULL; \
	uint32_t temp_count   = 0;

    
/** @brief A helper macro to standardize c-string parsing.
 *
 * If @p member is @ref CSM_NO_MEMBER, this macro will populate the array itself.
 * 
 * @param[in]  dsv_string        A c-string (char*) containig a list of delimiter separated values.
 * @param[in]  dsv_separator     A string specifying a colloction of separators.
 * @param[out] array             An array to store the separated list in, assumed to be allocated.
 * @param[in]  array_size        The size of the allocated array.
 * @param[in]  member            The member value of the @p array, if a flat array use @ref CSM_NO_MEMBER.
 * @param[in]  default_val       The default value to assign to the array members.
 * @param[in]  conversion        A single parameter function to process a substring.
 */
#define csm_parse_dsv_to_struct( dsv_string, dsv_separator, array, array_size, member, default_val, conversion ) \
    i = 0;                                                                                        \
    nodeStr = strtok_r(dsv_string, dsv_separator, &saveptr);                                      \
                                                                                                  \
    while ( nodeStr != NULL && i < array_size )                                                   \
    {                                                                                             \
        array[i++]member = conversion(nodeStr);                                                   \
        nodeStr = strtok_r(NULL, dsv_separator, &saveptr);                                        \
    }                                                                                             \
    while ( i < array_size )                                                                      \
    {                                                                                             \
        array[i++]member = default_val;                                                           \
    }


/** @brief A helper macro to standardize c-string parsing into an array of structs.
 *
 * If @p member is @ref CSM_NO_MEMBER, this macro will populate the array itself.
 * 
 * @param[in]  csv_string        A c-string (char*) containig a list of comma separated values.
 * @param[out] array             An array to store the separated list in, assumed to be allocated.
 * @param[in]  array_size        The size of the allocated array.
 * @param[in]  member            The member value of the @p array, if a flat array use @ref CSM_NO_MEMBER.
 * @param[in]  default_val       The default value to assign to the array members.
 * @param[in]  conversion        A single parameter function to process a substring.
 */
#define csm_parse_csv_to_struct( csv_string, array, array_size, member, default_val, conversion ) \
    csm_parse_dsv_to_struct( csv_string, ",", array, array_size, member, default_val, conversion)

/** @brief A helper macro to parse postgresql arrays.
 *
 * If @p member is @ref CSM_NO_MEMBER, this macro will populate the array itself.
 *
 * @warning doesn't handle internal commas(,), braces({}) or quotes(").
 *
 * @param[in]  psql_array  A c-string (char*) containing a postgres formatted array.
 * @param[out] array       An array to store the separated list in, assumed to be allocated.
 * @param[in]  array_size  The size of the allocated array.
 * @param[in]  member      The member value of the @p array, if a flat array use @ref CSM_NO_MEMBER.
 * @param[in]  default_val The default value to assign to the array members.
 * @param[in]  conversion  A single parameter function to process a substring.
 */
#define csm_parse_psql_array_to_struct( psql_array, array, array_size, member, default_val, conversion ) \
    csm_parse_dsv_to_struct( psql_array,  ",\"{}", array, array_size, member, default_val, conversion )

/** @brief A helper macro for c-string trimming. Removes first and last char.
 * 
 * @param[in]  raw_string        A c-string (char*) that has a container. IE ( {my_string} or "mystring" )
 */
#define csm_trim_array_string( raw_string ) \
	field_length = strlen(raw_string);                                                            \
	temp_trimmed = (char*)malloc(field_length-1);                                                 \
	memcpy(temp_trimmed, (raw_string)+1, field_length-2);                                         \
	temp_trimmed[field_length-2] = '\0';
	
/**
 * @brief NULLS and frees the variables after using @ref csm_trim_array_string.
 * Declares i, saveptr and nodeStr.
 */
#define csm_free_trim() \
	free(temp_trimmed);         \
    temp_trimmed = NULL;        \
	field_length = 0;
	
/**
 * @brief resets helper vars for the sub struct macro below.
 */
#define csm_sub_struct_reset_data() \
    j = 0;               \
	field_length = 0;    \
	temp_trimmed = NULL; \
	temp_strings = NULL; \
	temp_32s     = NULL; \
	temp_64s     = NULL; \
	temp_doubles = NULL; \
	temp_count   = 0;
	
/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_string( db_data, parent_struct, parent_struct_count, struct_field_name) \
    /* trim */                                                                                             \
	csm_trim_array_string( db_data );                                                              \
	/* allocate num parent_struct worth */                                                                 \
	temp_strings = (char**)calloc(temp_count, sizeof(char*));                                              \
	/* Parse the struct_field_name. */                                                                     \
	csm_parse_csv_to_struct(temp_trimmed, temp_strings, temp_count, CSM_NO_MEMBER, strdup("N/A"), strdup); \
	/* copy to struct */                                                                                   \
	for(j = 0; j < o->parent_struct_count; j++){                                                           \
		o->parent_struct[j]->struct_field_name = temp_strings[j];                                          \
		temp_strings[j] = NULL;                                                                            \
		free(temp_strings[j]);                                                                             \
	}                                                                                                      \
	temp_strings = NULL;                                                                                   \
	free(temp_strings);                                                                                    \
	csm_free_trim();     

/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_int32( db_data, parent_struct, parent_struct_count, struct_field_name) \
	/* trim */                                                                                       \
	csm_trim_array_string( db_data );                                                        \
	/* allocate num parent_struct worth */                                                           \
	temp_32s = (int32_t*)calloc(temp_count, sizeof(int32_t));                                        \
	/* Parse the struct_field_name. */                                                               \
	csm_parse_csv_to_struct(temp_trimmed, temp_32s, temp_count, CSM_NO_MEMBER, -1, csm_to_int32);    \
	/* copy to struct */                                                                             \
	for(j = 0; j < o->parent_struct_count; j++){                                                     \
		o->parent_struct[j]->struct_field_name = temp_32s[j];                                        \
		temp_32s[j] = -1;                                                                            \
	}                                                                                                \
	temp_32s = NULL;                                                                                 \
	free(temp_32s);                                                                                  \
	csm_free_trim();	
	
/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_int64( db_data, parent_struct, parent_struct_count, struct_field_name) \
	/* trim */                                                                                       \
	csm_trim_array_string( db_data );                                                        \
	/* allocate num parent_struct worth */                                                           \
	temp_64s = (int64_t*)calloc(temp_count, sizeof(int64_t));                                        \
	/* Parse the struct_field_name. */                                                               \
	csm_parse_csv_to_struct(temp_trimmed, temp_64s, temp_count, CSM_NO_MEMBER, -1, csm_to_int64);    \
	/* copy to struct */                                                                             \
	for(j = 0; j < o->parent_struct_count; j++){                                                     \
		o->parent_struct[j]->struct_field_name = temp_64s[j];                                        \
		temp_64s[j] = -1;                                                                            \
	}                                                                                                \
	temp_64s = NULL;                                                                                 \
	free(temp_64s);                                                                                  \
	csm_free_trim();
	
/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_char( db_data, parent_struct, parent_struct_count, struct_field_name) \
	/* trim */                                                                                             \
	csm_trim_array_string( db_data );                                                              \
	/* allocate num parent_struct worth */                                                                 \
	temp_strings = (char**)calloc(temp_count, sizeof(char*));                                              \
	/* Parse the serial numbers. */                                                                        \
	csm_parse_csv_to_struct(temp_trimmed, temp_strings, temp_count, CSM_NO_MEMBER, strdup("N/A"), strdup); \
	/* copy to into each ssd struct */                                                                     \
	for(j = 0; j < o->parent_struct_count; j++){                                                           \
		/* copy the single char for struct_field_name*/                                                    \
		o->parent_struct[j]->struct_field_name = temp_strings[j][0];                                       \
		temp_strings[j] = NULL;                                                                            \
		free(temp_strings[j]);                                                                             \
	}                                                                                                      \
	temp_strings = NULL;                                                                                   \
	free(temp_strings);                                                                                    \
	csm_free_trim();

/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_double( db_data, parent_struct, parent_struct_count, struct_field_name) \
	/* trim */                                                                                         \
	csm_trim_array_string( db_data );                                                 \
	/* allocate num parent_struct worth */                                                                      \
	temp_doubles = (double*)calloc(temp_count, sizeof(double));                                        \
	/* Parse the struct_field_name. */                                                                \
	csm_parse_csv_to_struct(temp_trimmed, temp_doubles, temp_count, CSM_NO_MEMBER, -1, csm_to_double); \
	/* copy to struct */                                                                               \
	for(j = 0; j < o->parent_struct_count; j++){                                                                \
		o->parent_struct[j]->struct_field_name = temp_doubles[j];                                              \
		temp_doubles[j] = -1.0;                                                                        \
	}                                                                                                  \
	temp_doubles = NULL;                                                                               \
	free(temp_doubles);                                                                                \
	csm_free_trim();
	
/**
 * @brief NAQD helper macro. parses DB data into sub struct
 */
#define csm_sub_struct_dbtimestamp( db_data, parent_struct, parent_struct_count, struct_field_name) \
	/* trim */                                                                                             \
	csm_trim_array_string( db_data );                                                              \
	/* allocate num parent_struct worth */                                                                 \
	temp_strings = (char**)calloc(temp_count, sizeof(char*));                                              \
	/* Parse the struct_field_name. */                                                                     \
	csm_parse_csv_to_struct(temp_trimmed, temp_strings, temp_count, CSM_NO_MEMBER, strdup("N/A"), strdup); \
	/* copy to struct */                                                                                   \
	for(j = 0; j < o->parent_struct_count; j++){                                                           \
		/*do some funky stuff with the DB return */                                                        \
		csm_free_trim();                                                                                   \
		csm_trim_array_string( temp_strings[j] );                                                  \
		/*normal things*/                                                                                  \
		o->parent_struct[j]->struct_field_name = strdup(temp_trimmed);                                     \
		temp_strings[j] = NULL;                                                                            \
		free(temp_strings[j]);                                                                             \
	}                                                                                                      \
	temp_strings = NULL;                                                                                   \
	free(temp_strings);                                                                                    \
	csm_free_trim();

/**@brief Sets the verbose mode of the API call.
 *
 * @param[in] optarg The optarg being parsed for verbosity.
 * @param[in] usage  A function to display usage, typically in the format of:
 *                      csm_free_struct_ptr(STRUCT_TYPE, struct); help
 */
#define csm_set_verbosity( optarg, usage )                                          \
    if( !optarg ||  ( strcmp(optarg,"off")      != 0 &&                         \
    		          strcmp(optarg,"trace")    != 0 &&                         \
    		          strcmp(optarg,"debug")    != 0 &&                         \
    		          strcmp(optarg,"info")     != 0 &&                         \
    		          strcmp(optarg,"warning")  != 0 &&                         \
    		          strcmp(optarg,"error")    != 0 &&                         \
    		          strcmp(optarg,"critical") != 0 &&                         \
    		          strcmp(optarg,"always")   != 0 &&                         \
    		          strcmp(optarg,"disable")  != 0)){                         \
    	csmutil_logging(error,                                                  \
            "Invaild parameter for -v: optarg , Encountered: %s", optarg);      \
    	usage();                                                                \
    	return CSMERR_INVALID_PARAM;                                            \
    }                                                                           \
    csmutil_logging_level_set(optarg);                                          \
    csmutil_logging(always, "logging level set to: %s", optarg);

/**@brief Determines if the optarg is not null, returns @ref CSMERR_INVALID_PARAM if it is.
 *
 * @param[in] opt    The option that was being parsed.
 * @param[in] optarg The optarg being tested for existence.
 * @param[in] usage  A function to display usage, typically in the format of:
 *                      csm_free_struct_ptr(STRUCT_TYPE, struct); help
 */
#define csm_optarg_test( opt, optarg, usage )                           \
    if ( !optarg ) {                                                \
        csmutil_logging(error, "Invalid parameter for %s :", opt);  \
        usage();                                                    \
        return CSMERR_INVALID_PARAM;                                \
    }


// ==============================================================
// Function Prototypes
// ==============================================================

// Used in concatination of function names using macros.
/// Helper macro for other macros.
#define CSM_FUNCT_CAT(A, B) CSM_FUNCT_CAT_(A,B)
/// Helper macro for other macros.
#define CSM_FUNCT_CAT_(A, B) A##B


// ==============================================================
// Function Macro Calls - Helper Macros to clean up the code.
// ==============================================================

/** @brief Retrieves the integer value of a string for an enumerated type.
 *  
 *  @param[in] ENUM_TYPE The type of the enum to check.
 *  @param[in] string char*: The string to convert to an enum type.
 *
 *  @return -1 The string could not be found.
 *  @return The int version of the enum member matching the string.
 */
#define csm_get_enum_from_string( ENUM_TYPE, string )\
    csm_enum_from_string( string, ENUM_TYPE##_strs )

/** @brief Retrieves the string from the array associated with the enumerated value.
 * @param     ENUM_TYPE The type of enum.
 * @param[in] value ENUM_TYPE: The value of the enum to get the string for.
 *
 * @return NULL The enum value was invalid.
 * @return char* The string associated with the value.
 */
#define csm_get_string_from_enum( ENUM_TYPE, value )\
   (value >= 0 && value < ENUM_TYPE##_MAX) ? ENUM_TYPE##_strs[value] : ""

/** @brief Retrieves the integer value of a string for an enumerated type.
 *
 * Performs a bitwise shift to convert the array index to the enum value.
 * This should only be used on enums that have a bit_count definition.
 *
 *  @param[in] ENUM_TYPE The type of the enum to check.
 *  @param[in] string char*: The string to convert to an enum type.
 *  @param[out] enum_val int: An integer to store the enum value in.
 *          
 */
#define csm_get_enum_bit_flag( ENUM_TYPE, string, enum_val)\
    int enum_id = csm_enum_from_string(string, ENUM_TYPE##_strs );\
    enum_val = (enum_id > 0 && enum_id <= csm_enum_bit_count(ENUM_TYPE)) ? \
        1 << (enum_id-1): enum_id > 0 ? csm_enum_max(ENUM_TYPE) - 1: enum_id;

/** @brief Returns the maximum value of the enumerated type.
 *
 * @param ENUM_TYPE The name of the enumerated type to get the maximum value of.
 *
 * @return Maximum enum value for the supplied type.
 */
#define csm_enum_max(ENUM_TYPE) ENUM_TYPE##_MAX

/** @brief The number of bits for a flag type enum.
 *
 * @param ENUM_TYPE The name of the enumerated type to get the bit count of.
 *
 * @return Number of bits this flag is designed to pack.
 */
#define csm_enum_bit_count(ENUM_TYPE) ENUM_TYPE##_bit_count

/** @brief Initializes the versioning details for the struct.
 *
 *  @param[in] target A struct with a `_metadata` field, populated with @ref CSM_VERSION_ID.
 */
#define csm_init_struct_versioning( target ) (target)->_metadata=CSM_VERSION_ID

/** @brief Initializes an already allocated struct.
 *
 *  @param     STRUCT_NAME The type of the struct.
 *  @param[in] target STRUCT_NAME| A struct that has been allocated of the type 
 *                  specified in @p STRUCT_NAME.
 */
#define csm_init_struct(STRUCT_NAME, target )\
    target._metadata=CSM_VERSION_ID;     \
    CSM_FUNCT_CAT(init_,STRUCT_NAME)( &target )

/** @brief Initializes a pointer to a struct.
 *  @warning This macro will allocate the struct on the heap!
 *
 *  @param     STRUCT_NAME The type of the struct.
 *  @param[in] target STRUCT_NAME*: A pointer to a struct of the type 
 *                  specified in @p STRUCT_NAME.
 */
#define csm_init_struct_ptr(STRUCT_NAME, target)             \
    target = (STRUCT_NAME*) malloc(sizeof(STRUCT_NAME)); \
    target->_metadata=CSM_VERSION_ID;                    \
    CSM_FUNCT_CAT(init_,STRUCT_NAME)( target )

/** @brief Frees a struct that has been allocated on the stack. 
 *
 *  @warning Only invoke if the corresponding @ref csm_init_struct_ptr was called.
 *
 *  @param     STRUCT_NAME          The type of the struct.
 *  @param[in] target STRUCT_NAME: A stack allocated struct of the type 
 *                  specified in @p STRUCT_NAME.
 */
#define csm_free_struct(STRUCT_NAME, target ) \
    CSM_FUNCT_CAT(free_,STRUCT_NAME)( &target )

/** @brief Frees a pointer to a struct allocated on the heap.
 *  @note This macro will free the @p target after freeing its contents and set it to NULL.
 *
 *  @param     STRUCT_NAME          The type of the struct.
 *  @param[in] target STRUCT_NAME*: A pointer to a heap allocated struct of the type 
 *                  specified in @p STRUCT_NAME.
 */
#define csm_free_struct_ptr(STRUCT_NAME, target ) \
    CSM_FUNCT_CAT(free_,STRUCT_NAME)( target );   \
    free(target);                             \
    target = NULL

/** @brief Serializes a pointer to a struct of the type specified in @p STRUCT_NAME.
 *
 *  @param      STRUCT_NAME           The type of the struct.
 *  @param[in]  target  STRUCT_NAME*: A pointer to a struct to be serialized.
 *  @param[out] buffer        char**: A buffer to place the results of the serialization.
 *  @param[out] buffer_len uint32_t*: A container for the length of the buffer.
 */
#define csm_serialize_struct(STRUCT_NAME, target, buffer, buffer_len ) \
    CSM_FUNCT_CAT(serialize_,STRUCT_NAME)( target, buffer, buffer_len)

/** @brief Deserializes a struct of type @p STRUCT_NAME from the supplied @p buffer.
 *  
 *  @param      STRUCT_NAME           The type of the struct.
 *  @param[out] dest  STRUCT_NAME**:  The destination of the deserialization.
 *  @param[in]  buffer  const char*:  The buffer containing the struct in serial form.
 *  @param[in]  buffer_len uint32_t:  The length of the buffer.
 */
#define csm_deserialize_struct( STRUCT_NAME, dest, buffer, buffer_len ) \
    CSM_FUNCT_CAT(deserialize_,STRUCT_NAME)(dest, buffer, buffer_len) 

/** @} */

#endif
