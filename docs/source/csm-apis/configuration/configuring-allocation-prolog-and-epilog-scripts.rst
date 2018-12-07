Configuring allocation prolog and epilog scripts
================================================

A *privileged_prolog* and *privileged_epilog* script (with those exact names) must be placed in ``/opt/ibm/csm/prologs`` on a compute node in order to use the *csm_allocation_create*, *csm_allocation_delete*, and *csm_allocation_update* APIs. These scripts must be executable and take three command line parameters: *--type*, *--user_flags*, and *--sys_flags*. 

To add output from this script to the Big Data Store (BDS) it is recommended that the system administrator producing these scripts make use of their language of choice’s logging function. 

A sample *privileged_prolog* and *privileged_epilog* written in python is shipped in **ibm-csm-core** at ``/opt/ibm/csm/share/prologs``. These sample scripts demonstrate the use of the python logging module to produce logs consumable for the BDS.

Table 1 Mandatory prolog/epilog features:

+----------------------+------------------------------------------------------------------+
| Feature              | Description                                                      |
+======================+==================================================================+
| --type               | The script must accept a command line parameter --type and have  |
|                      | support for both allocation and step as a string value.          |
|                      |                                                                  |
+----------------------+------------------------------------------------------------------+
| --sys_flags          | The script must have a command line parameter --sys_flags. This  |
|                      | parameter should take a space delimited list of alphanumeric     |
|                      | flags in the form of a string. CSM does not allow special        |
|                      | characters, as these represent a potential exposure, allowing    |
|                      | unwanted activity to occur.                                      |
+----------------------+------------------------------------------------------------------+
| --user_flags         | The script must have a command line parameter –user_flags. This  |
|                      | parameter should take a space delimited list of alphanumeric     |
|                      | flags in the form of a string. CSM does not allow special        |
|                      | characters, as these represent a potential exposure, allowing    |
|                      | unwanted activity to occur.                                      |
+----------------------+------------------------------------------------------------------+
| Returns 0 on success | Any other error code will be captured by create/delete and the   |
|                      | api call will fail.                                              |
+----------------------+------------------------------------------------------------------+

Table 2 Optional prolog/epilog features:

+----------------------+------------------------------------------------------------------+
| Feature              | Description                                                      |
+======================+==================================================================+
| logging              | If the sysadmin wants to track these scripts in BDS, a form of   |
|                      | logging must be implemented by the admin writing the script. The |
|                      | sample scripts outline a technique using python and the logging  |
|                      | module.                                                          |
|                      |                                                                  |
+----------------------+------------------------------------------------------------------+

Table 3 Prolog/epilog environment variables:

+----------------------+------------------------------------------------------------------+--------------------+--------------+
| Environment Variable | Description                                                      | Allocation Support | Step Support |
+======================+==================================================================+====================+==============+
| CSM_ALLOCATION_ID    | The Allocation ID of the invoking CSM handler.                   | Yes                | Yes          |
+----------------------+------------------------------------------------------------------+--------------------+--------------+
| CSM_PRIMARY_JOB_ID   | The Primary Job (Batch) ID of the invoking CSM handler.          | Yes                | No           |
+----------------------+------------------------------------------------------------------+--------------------+--------------+
| CSM_SECONDARY_JOB_ID | The Secondary Job (Batch) ID of the invoking CSM handler.        | Yes                | No           |
+----------------------+------------------------------------------------------------------+--------------------+--------------+
| CSM_USER_NAME        | The user associated with the job.                                | Yes                | No           |
+----------------------+------------------------------------------------------------------+--------------------+--------------+

A *step prolog* or *step epilog* differs in two ways: the *–type flag* is set to *step* and certain environment variables will not be present. Please refer to Table 1 Mandatory prolog/epilog features and Table 3 Prolog/epilog environment variables for more details.































