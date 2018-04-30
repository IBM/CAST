/*================================================================================

    csmi/src/wm/cmd/allocation_create_man.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#define HELP_CSM_ALLOCATION_CREATE

_____CSM_ALLOCATION_CREATE_CMD_HELP_____
USAGE:
  csm_allocation_create ARGUMENTS [OPTIONS]
  csm_allocation_create -j primary_job_id -n \"node01,node02\" [-J secondary_job_id] [-s state] [-t type] [-u user_name] [-U user_id] [-h] [-v verbose_level] [--create_cgroup] [--shared]

SUMMARY: Used to create an allocation.

EXIT STATUS:
  0  if OK,
  1  if ERROR.

ARGUMENTS:
  MANDATORY:
    csm_allocation_create expects 2 mandatory arguments
    Argument                | Example value      | Description
    ------------------------|--------------------|--------------
    -j, --primary_job_id    | 1                  | (LONG INTEGER) Primary job id [>0] (for lsf this will be the lsf job id).
                            |                    | 
    -n, --node_range        | \"node01,node02\"  | (STRING) The node_names to be allocated. Separated by comma.
                            |                    | (at least 1 node (node01,node02,...) is required)
                            |                    | 
  OPTIONAL:
    csm_allocation_create can have 5 optional arguments
    Argument               | Example value   | Description
    -----------------------|-----------------|--------------
    -J, --secondary_job_id | 0               | (INTEGER) Secondary job id (for lsf this will be the lsf job index for job arrays) [>=0].
                           |                 | (default = '0')
                           |                 | 
    -s, --state            | \"running\"     | (STRING) The state of the allocation.
                           |                 | (default = \"running\")
                           |                 | Valid values: \"staging in\", \"running\"
                           |                 | 
    -t, --type             | \"user managed\"| (STRING) The type of allocation.
                           |                 | (default = \"user managed\")
                           |                 | Valid values: \"shared\", \"user managed\", \"pmix\", \"pmix with cgroups\", or \"diagnostics\"
                           |                 | 
    -u, --user_name        |                 | (STRING) The owner of this allocation's linux user name.
                           |                 | (default is invoking user)
                           |                 | 
    -U, --user_id          | 0               | (INTEGER) The owner of this allocation's linux user id.
                           |                 | 
    --isolated_cores          | 0               | (INTEGER) Specifies the type of cgroup configuration for the node: 0 - none, 1 - Allocation, 2 - Allocation and Core Isolation.
    --shared               |                 | (FLAG) This type allocation may be shared.

GENERAL OPTIONS:
[-h, --help]                  | Help.
[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}

EXAMPLE OF USING THIS COMMAND:

