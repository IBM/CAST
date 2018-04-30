# Header Generation Overview

The CSM APIs use an X Macro based solution to handle serialization, memory management
and initialization of the structs. For details on the file format used in the CSM X Macros
please consult the READMEs in the `csm_types` directory.

A generation script has been implemented to interpret the CSM def files to generate documented
headers (decoupled from the def files) for distribution. Additionally, this generation script
will produce the headers for the functions used to manipulate the structs and the implementations
of these functions (which are unexpanded X Macros for space).

As of GA the generation scripts will also enforce backwards compatibility in the CSM structs.

The primary generation script is invoked through `regenerate_headers.sh`, `-h` is supported for
an overview of options that may be supplied to the script. The following files must be in the 
same directory as the `regenerate_headers.sh` script. If any of the following files are missing
the script will not execute with the desired/supported behavior.

* `regenerate_headers.sh`
  * Associated Script files
    * `regenerate_headers_constants.sh`
    * `regenerate_headers_functions.sh`
    * `regenerate_headers_text_functs.sh`
    * `function_parse.pl`
    * `generate_struct.pl`
    * `generate_enum.pl`
  * Template Files
    * `c_funct.txt`
    * `header_funct.txt`
    * `header_funct_alias.txt`
    * `header_funct_comments.txt`
    * `header_funct_def.txt`
    * `internal_header_funct_def.txt`
    * `internal_header_funct_comments.txt`


Each of these files has a discrete purpose in the generation of the CSM headers.

# regenerate_headers.sh

This script is the entry point to the 

