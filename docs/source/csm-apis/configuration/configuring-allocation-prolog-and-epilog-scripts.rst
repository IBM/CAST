Configuring allocation prolog and epilog scripts
================================================

A privileged_prolog and privileged_epilog script (with those exact names) must be placed in /opt/ibm/csm/prologs on a compute node in order to use the csm_allocation_create, csm_allocation_delete and csm_allocation_update APIs. These scripts must be executable and take three command line parameters: --type, --user_flags, and --sys_flags. 

