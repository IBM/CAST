# CSM API Overview # 

# Cluster System Manager # {#csm}

## Authors ## {#csmauthors}
- John Dunham (jdunham@us.ibm.com)
- Nicholas Buonarota (nbuonar@us.ibm.com)

## Overview ## {#csmoverview}

The Cluster System Manager(CSM) APIs is intended to provide a well defined interace to 
functionality offered by the various @ref csmcomponents. The CSM APIs will be provided as 
a set of C language libraries and header files.

## CSM Components ## {#csmcomponents}

The currently supported and planned components in CSM include the following:

- [Workload Management](@ref wm_apis)
- [Burst Buffer](@ref bb_apis)
- [Inventory](@ref inv_apis)
- [Diagnostics](@ref diag_apis)
- [RAS](@ref ras_apis)
- Launch

For detailed information on each of the APIs please consult the above sections.

## Usage ## {#csmusage}

### C APIs ### {#csmcapis}

The CSM APIs are grouped based on component, with each component having three separate headers 
in the include directory. 

The @ref csm_api.h header is a special header which includes all of the @ref csmdatatypehelpers
header files. 

| Component           | Functions                       | Data Types            | Data Type Helpers           |
|---------------------|---------------------------------|-----------------------|-----------------------------|
| Workload Management | @ref csm_api_workload_manager.h | @ref csmi_type_wm.h   | @ref csmi_type_wm_funct.h   |
| Burst Buffer        | @ref csm_api_burst_buffer.h     | @ref csmi_type_bb.h   | @ref csmi_type_bb_funct.h   |
| Inventory           | @ref csm_api_inventory.h        | @ref csmi_type_inv.h  | @ref csmi_type_inv_funct.h  |
| Diagnostics         | @ref csm_api_diagnostics.h      | @ref csmi_type_diag.h | @ref csmi_type_diag_funct.h |
| RAS                 | @ref csm_api_ras.h              | @ref csmi_type_ras.h  | @ref csmi_type_ras_funct.h  |
| Launch              | TBD                             | TBD                   | TBD                         |

#### Functions #### {#csmfunctions}

The @ref csmfunctions headers define functions to interact with the CSM component. Each header
includes the @ref csmdatatypehelpers header, which provides the helper functions to the structs
and the @ref csmdatatypes. 

Before invocation of these functions may be performed the user needs to initialize the library
connection and prepare an @ref csm_api_object as outlined in @ref csm_api_common.h.

For detailed descriptions and listings of available functions in the API please consult either
the module matching the desired API or the header referenced in the table above.
 
#### Data Types #### {#csmdatatypes}

The @ref csmdatatypes headers define the structs, enums and enum strings for the component.
Most functions have one to two related structs. For detailed information on the structs
please consult the documentation in the header files.

#### Data Type Helper #### {#csmdatatypehelpers}

The @ref csmdatatypehelpers headers define helpr functions for initializing and destruction of 
structs. These functions are not mandatory, however, it is recommended that if the 
user does not use the **init** functions of the struct a **calloc** call is used so all 
struct pointers are zeroed for serialization purposes.

A set of macros is defined in @ref csm_api_helper_macros.h to assist in using the 
@ref datatypehelpers headers files.

#### Common Header #### {#csmcommon}

A header is provided (@ref csmi_type_common.h) which provides the error codes that may be 
produced by the CSM APIs, the @ref csm_api_object, and the 
[library functions](@ref csm_api_common.h). Generally speaking this header is already included
by the other components, however, it is a vital reference for API operation.

### Command Line Interface ### {#csmcli}
