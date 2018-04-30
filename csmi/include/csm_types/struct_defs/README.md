# CSM Serialization Guide # 

CSM uses a collection of `def` files, separated by module, to specify structs. These `def` files
are used in conjunction with X-Macros to generate initialization, serialization and free functions.

To add a struct to CSM the following steps must be preformed:

1. Create a `def` file in the appropriate module directory.
2. Add the `def` file name to the `type_order.def` file.
    * If the struct needs any special preprocessor directives add them to `special_preprocess.def`.
3. Run `csmi/include/struct_generator/regenerate_headers.sh`.

## Def File ##
A `def` file may specify a struct and its functions in CSM. CSM structs may be located at 
`csmi/include/csm_types/struct_defs`. This directory contains several sub directories, each
representing a separate `module` of CSM.

Directory | Type Header              | Function Header
----------|--------------------------|-----------------
bb        | csmi_type_bb.h           | csmi_type_bb_funct.h
common    | csmi_type_common.h       | csmi_type_common_funct.h
diag      | csmi_type_diag.h         | csmi_type_diag_funct.h
inv       | csmi_type_inv.h          | csmi_type_inv_funct.h
launch    | csmi_type_launch_funct.h | csmi_type_launch_funct.h
ras       | csmi_type_ras.h          | csmi_type_ras_funct.h
wm        | csmi_type_wm.h           | csmi_type_wm_funct.h

### Macro Definitions ###
As this technique leverages X-Macros to meta program the serialization code there are several macros 
that should be defined (or undefined). Macros can directly influence the struct, or be used to give 
hints to the code generation in function generation.

#### Struct Definitions ####
The following definitions directly affect the struct:

Definition       | Description
-----------------|-------------
CSMI_STRUCT_NAME | The name of the struct, **required** to generate the struct. *Use this definition to guard the defines.*
CSMI_STRUCT_ALIAS| A previously defined struct (using this technique), **optional**. If specified, no additional defines are needed.

It is **required** that the programmer wrap the macro definitions in an `ifndef` block using 
`CSMI_STRUCT_NAME`. The X-Macros are written with the assumption that the macro definitions looks 
something like this:

```C
#ifndef CSMI_STRUCT_NAME
    #define CSMI_STRUCT_NAME csm_struct_name_t
    // Additional macro definitions
#endif
```

If `CSMI_STRUCT_ALIAS` is set the generated structure will look something like this:
``` C
typedef CSMI_STRUCT_ALIAS CSMI_STRUCT_NAME;
```
Additionally, any generated functions will be macro aliases to the existing struct's generated functions.

#### Function Type Definitions ####
In addition to macros that directly influence the definition of the struct, there are several 
**required** macro definitions used in function generation:

Definition              | Description
------------------------|---------------------------------------------------------
CSMI_BASIC              | Anything that can use a memcopy ( uint32_t, char, etc. ).
CSMI_STRING             | A C string ( char\* ), null terminated.
CSMI_STRING_FIXED       | A fixed length C string ( char var[fixed_length] ).
CSMI_ARRAY              | An array of a primitive types. <br/> 
CSMI_ARRAY_FIXED        | A fixed length array of primitive type. <br/>.
CSMI_ARRAY_STR          | An array of strings ( char\*\* ).
CSMI_ARRAY_STR_FIXED    | A fixed length array of strings ( char\* var[fixed_length] ). 
CSMI_STRUCT             | A C struct pointer.
CSMI_ARRAY_STRUCT       | An array of structs.
CSMI_ARRAY_STRUCT_FIXED | An array of structs, fixed length.
CSMI_NONE               | This field requires a custom solution. **NOT SUPPORTED**

Several of the above definitions have special rules that are used in the `CSMI_STRUCT_MEMBER` call:

Keyword       | Modifier
--------------|--------------
ARRAY         | The `length_member` field of the `CSMI_STRUCT_MEMBER` macro call must be set to a member above it.
ARRAY_*_FIXED | The `length_member` field of the `CSMI_STRUCT_MEMBER` macro call must be set to a compile time length
STRUCT        | The `extra` field of the `CSMI_STRUCT_MEMBER` macro call must be the name of the struct (no pointer).

When writing macro definition for the def file it is strongly recommended that the programmer 
`#undef` all of the above definitions to prevent collisions or leftover definitions:

```C
    #undef CSMI_BASIC
    #undef CSMI_STRING
    #undef CSMI_STRING_FIXED
    #undef CSMI_ARRAY
    #undef CSMI_ARRAY_FIXED
    #undef CSMI_ARRAY_STR
    #undef CSMI_ARRAY_STR_FIXED
    #undef CSMI_STRUCT
    #undef CSMI_ARRAY_STRUCT
    #undef CSMI_ARRAY_STRUCT_FIXED
    #undef CSMI_NONE
```

Then the programmer should define the type definitions needed. 
`1` indicates that the struct has a member matching that type.
`0` indicates that the struct does not have a member matching that type.

``` C
    #define CSMI_BASIC               1
    #define CSMI_STRING              1
    #define CSMI_STRING_FIXED        0
    #define CSMI_ARRAY               1
    #define CSMI_ARRAY_FIXED         0
    #define CSMI_ARRAY_STR           0
    #define CSMI_ARRAY_STR_FIXED     0
    #define CSMI_STRUCT              0
    #define CSMI_ARRAY_STRUCT        0
    #define CSMI_ARRAY_STRUCT_FIXED  0
    #define CSMI_NONE                0
```

In the above example the struct would have one or more strings, primitive types and array of primitives.

### Adding Struct Members ###
After defining the macros and ending the definition block the programmer can specify the members of the struct.

CSM's X-Macro call is `CSMI_STRUCT_MEMBER`. It takes six parameters with the following format:
```C
    CSMI_STRUCT_MEMBER(type, name, serial_type, length_member, init_value, extra) /**< Comment */
```
Parameter     | Description
--------------|---------------
type          | The `C` type of the struct member (e.g. `int64_t`, `char*`, `struct_t**`).
name          | The name of the member, `C` naming conventions are required.
serial_type   | The serial type of the member for the purposed of serialization. Uses the definitions from [above](#Function Type Definitions) dropping the `CSMI_`.
length_member | The length of an `ARRAY` or `FIXED` member. In the case of arrays this typically points to another member above it. Fixed members require a compile time value.
init_value    | The initial value of the member for an initializer, only supported in `BASIC` members.
extra         | Fulfills the need of the member, typically the `type` field without pointers,  more details specified [above](#Function Type Definitions) in the keyword table.

Additionally, a member may specify an inline comment (as shown above) which will be transferred to the generated struct for the purposes of documentation. These comments must be single line at the time of writing.

``` C
CSMI_STRUCT_MEMBER( uint32_t, num_allocations, BASIC, , 0, ) /**< Number of allocations. */
CSMI_STRUCT_MEMBER( csmi_allocation_t**, allocations, ARRAY_STRUCT, num_allocations, NULL, csmi_allocation_t) /**< Active allocations. */
```

The above sample would generate a struct with the body:
``` C
{
    uint32_t num_allocations; /**< Number of allocations. */
    csmi_allocation_t** allocations; /**< Active allocations. */
}
```
The struct functions would then initialize `num_allocations` to 0 and use it as the length member 
to `allocations`.

**ATTENTION:** When all of the members have been specified add `#undef CSMI_STRUCT_MEMBER` to the 
end of the file. This cleans up the macro for the code generation.

#### Versioning Structs ####
CSM provides a mechanism for versioning structs in the X-Macro definitions.

Definition         | Description
-------------------|-------------
CSMI_VERSION_START | Indicates the start of a CSM version block, this takes a version id defined in
                   | `csm_api_version.h`. Typically this should have CSM_DEVELOPMENT as a parameter.
CSMI_VERSION_END   | Indicates the end of a CSM version block, this will be populated by `regenerate_headers.sh`
                   | and should be set to zero during development. The populated value is the hashed
                   | value of the struct to that point and is used to verfiy that no changes have 
                   | occured to released fields.

This version block will typically look like the following:

``` C
    CSMI_VERSION_START(CSM_DEVELOPMENT)
    CSMI_STRUCT_MEMBER(...)
    CSMI_STRUCT_MEMBER(...)
    ...
    CSMI_VERSION_END(0)
```

This block mechanism will have an impact on the fields serialized and deserialized by CSM when 
communicating between daemons. The serializers/deserializers will use the contents of the `_metadata`
field which should be set to a CSM version id. A convenience macro `csm_init_struct_versioning`
has been provided to ensure that the metadata field is set correctly for CSM to process.

Once a field is considered "released" by CSM the `regenerate_headers.sh` script should be executed
using the `-f <#.#.#>` option which will finalize the CSM struct and perform hash computations to
ensure backwards compatibility.

### Struct Comments ###
To comment on the struct add a multiline comment block to the `def` file with the first line being `CSMI_COMMENT`. 
The `regenerate_headers.sh` script will scrape the comments into the struct header.
``` C
/**
 * CSMI_COMMENT
 * @brief Defines a CSM Allocation in the CSM Database.
 *
 * The fields defined in this struct represent an entry in the *csm_allocation* table. If this
 * allocation represents a historic allocation (e.g. not active) the @ref csmi_allocation_t.history
 * field will be populated. The resultant combination will represent an entry in the
 * *csm_allocation_history* table.
 *
 * The @ref csmi_allocation_t.compute_nodes field represents any nodes that have participated in
 * the allocation, with the size of this array being represented by @ref csmi_allocation_t.num_nodes.
 */
```

### Internal Structs ###

The CSM X-Macro solution provides a mechanism for specifying a struct that is not exposed in the
public API but benefits from the serialization mechanism. To enable this feature simply add a 
line to the def file with `CSMI_INTERNAL` spcified. It is recommended to place this in a comment:

``` C
/** 
 * CSMI_INTERNAL
 * CSMI_COMMENT
...
```

## Special Files ##
Each module directory should contain two special files used by `regenerate_headers.sh`

### type_order.def ###
The `special_preprocess.def` file is a newline delimited list of `def` files which specifies
the order in which structs should be placed in their respective generated headers. For example:

```C 
csmi_allocation_history.def
csmi_allocation.def
csmi_allocation_mcast_context.def
csmi_allocation_mcast_payload.def
csmi_allocation_accounting.def
```

### special_preprocess.def ###
The `special_preprocess.def` file is a regular file, specifying any macros or includes that a module's 
type header may need. The contents of this file are placed directly in the generated header with no alterations.

 
