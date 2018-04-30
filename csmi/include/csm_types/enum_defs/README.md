# CSM Enum Serialization Guide # 

CSM uses a collection of `def` files, separated by module, to specify enumerated types.

To add an enumerated type to CSM the following steps must be preformed:

1. Create a `def` file in the appropriate module directory.
2. Add the `def` file name to the `type_order.def` file.
3. Run `csmi/include/struct_generator/regenerate_headers.sh`.

Enumerated types are always placed at the top of the generated type and function files.

## Def File ## 
A `def` file may specify a enumerated type in CSM. CSM structs may be located at 
`csmi/include/csm_types/enum_defs`. This directory contains several sub directories, each 
representing a separate `module` of CSM.

Directory | Type Header              
----------|--------------------------
bb        | csmi_type_bb.h           
common    | csmi_type_common.h       
diag      | csmi_type_diag.h         
inv       | csmi_type_inv.h          
launch    | csmi_type_launch_funct.h 
ras       | csmi_type_ras.h          
wm        | csmi_type_wm.h        

### Macro Definitions ###
CSM leverages X-Macros to generate the enumerated types in CSM and their string lists. 
Several macro definitions are required to achieve this.

Definition     | Description
---------------|--------------------------
CSMI_ENUM_NAME | **Required:** The name of the enumerated type.
CSMI_BIT_ENUM  | **Optional:** Specifies that the enum will use bit shifts. Adds a `NONE` and `ALL` member with the value of `CSMI_BIT_ENUM` prepending the members.

It is **required** that the programmer wrap the macro definitions in an `ifndef` block using 
`CSMI_ENUM_NAME`. The X-Macros are written with the assumption that the macro definitions looks 
something like this:
```C
#ifndef CSMI_ENUM_NAME
    #define CSMI_ENUM_NAME enum_t
    // Additional macros
#endif
```
The enumerated type will have an additional member `<CSMI_ENUM_NAME>_MAX` (string value: "") which 
is one past the last defined enumerated type. It is used for testing that an enum value is legal. 

The enumerated type name is also used to generate an array, which is terminated with a null string 
and populated by the `member_string` parameters:
```C
    extern const char* <CSMI_ENUM_NAME>_strs [];
```

If `CSMI_BIT_ENUM` the generated enum values will be compatible with bit shifting (e.g. powers of 2). 
Additionally, two additional members will be generated `<CSMI_BIT_ENUM>NONE` and `<CSMI_BIT_ENUM>ALL`. 
`NONE` evaluates to zero (string value: "NONE") and `ALL` evaluates to a bitwise or on all of the 
other members (string value: "ALL:"). `enum_num` values will be ignored if `CSMI_BIT_ENUM` is set.

```C
    // In def file
    #define CSMI_BIT_ENUM DIAG_
    ...
    // In Header File
    DIAG_NONE=0, ///< 0 - No flags set
    ...
    DIAG_ALL=31, ///< 31 - All flags set
```
    
At the bottom of the file the programmer must add `#undef CSMI_CSMI_BIT_ENUM` to prevent collisions 
with other enumerated types.

### Adding Enum Members ###
After defining the macros and ending the definition block the programmer can specify the members of 
the enumerated type.

CSM's X-Macro call is `CSMI_ENUM_MEMBER`. It takes four parameters with the following format:
```C
    CSMI_STRUCT_MEMBER(enum_member, member_string, enum_num, relationship) ///< Comment 
```
Parameter     | Description
--------------|---------------
enum_member   | The name of the enum member in `C`.
member_string | The string version of the member, used in generating the array.
enum_num      | The value of the enum member, empty for auto increment or bit shift.
relationship  | **UNUSED**

Additionally, a member may specify an inline comment (as shown above) which will be transferred to 
the generated enumerated type for the purposes of documentation. These comments must be single line 
at the time of writing.

``` C
CSMI_ENUM_MEMBER(CSM_NO_CGROUP        , "No cgroup"                  ,, ) ///< The allocation should create no cgroups.
CSMI_ENUM_MEMBER(CSM_ALLOC_CGROUP     , "Allocation cgroup"          ,, ) ///< The allocation should only create the allocation cgroup.
CSMI_ENUM_MEMBER(CSM_ALLOC_CORE_CGROUP, "Allocation and Core cgroup" ,, ) ///< The allocation creates both the allocation and core isolation cgroups.
```

The above sample would generate a struct with the body (**note** the bounding value is automatically added):
``` C
{
   CSM_NO_CGROUP=0, ///< 0 - The allocation should create no cgroups.
   CSM_ALLOC_CGROUP=1, ///< 1 - The allocation should only create the allocation cgroup.
   CSM_ALLOC_CORE_CGROUP=2, ///< 2 - The allocation creates both the allocation and core isolation cgroups.
   csmi_cgroup_type_t_MAX=3 ///< 3 - Bounding Value
}
```
And the array would be:
```C
    const char* csmi_cgroup_type_t_strs [] = {"No cgroup","Allocation cgroup","Allocation and Core cgroup",""};
```

**ATTENTION:** When all of the members have been specified add `#undef CSMI_ENUM_MEMBER` to the end 
of the file. This cleans up the macro for the code generation.

### Versioning Enums ###
CSM provides a mechanism for versioning enums in the X-Macro definitions.

Definition         | Description
-------------------|-------------
CSMI_VERSION_START | Indicates the start of a CSM version block, this takes a version id defined in
                   | `csm_api_version.h`. Typically this should have CSM_DEVELOPMENT as a parameter.
CSMI_VERSION_END   | Indicates the end of a CSM version block, this will be populated by `regenerate_headers.sh`
                   | and should be set to zero during development. The populated value is the hashed
                   | value of the enum to that point and is used to verfiy that no changes have
                   | occured to released fields.

This version block will typically look like the following:

``` C
    CSMI_VERSION_START(CSM_DEVELOPMENT)
    CSMI_STRUCT_MEMBER(...)
    CSMI_STRUCT_MEMBER(...)
    ...
    CSMI_VERSION_END(0)
```

Currently the versioning blocks exist to maintain backwards compatibility of the enumerated types.
The versioning is finalized by CSM using the `regenerate_headers.sh` script with the `-f <#.#.#>` option.

### Enumerated Type Comments ###
To comment on the Enumerated Type add a multiline comment block to the `def` file with the first 
line being `CSMI_ENUM_BRIEF`. The `regenerate_headers.sh` script will scrape the comments into the 
type header.

``` C
/**
 * CSMI_ENUM_BRIEF
 * @brief A Struct representing the cgroup configuration for when a job becomes running.
 *
 */
```

## Adding Enum 
## Special Files ##
Each module directory should contain one special file used by `regenerate_headers.sh`

### type_order.def ###
The `special_preprocess.def` file is a newline delimited list of `def` files which specifies
the order in which enums should be placed in their respective generated headers. For example:
```
csmi_state.def
csmi_cgroup_controller.def
csmi_cgroup_type.def
```

