.. _CSMProEpi:

Configuring allocation prolog and epilog scripts
================================================

A *privileged_prolog* and *privileged_epilog* script (with those exact names) must be placed in 
``/opt/ibm/csm/prologs`` on a compute node in order to use the *csm_allocation_create*, 
*csm_allocation_delete*, and *csm_allocation_update* APIs. These scripts must be executable and 
take three command line parameters: *--type*, *--user_flags*, and *--sys_flags*. 

To add output from this script to the Big Data Store (BDS) it is recommended that the system 
administrator producing these scripts make use of their language of choice’s logging function. 

A sample *privileged_prolog* and *privileged_epilog* written in python is shipped in 
|csm-core| at ``/opt/ibm/csm/share/prologs``. These sample scripts demonstrate the use of 
the python logging module to produce logs consumable for the BDS.

Mandatory prolog/epilog Features
--------------------------------

+----------------------+-----------------------------------------------------------------------+
| Feature              | Description                                                           |
+======================+=======================================================================+
| --type               | | The script must accept a command line parameter --type and have     |
|                      | | support for both allocation and step as a string value.             |
+----------------------+-----------------------------------------------------------------------+
| --sys_flags          | | The script must have a command line parameter --sys_flags. This     |
|                      | | parameter should take a space delimited list of alphanumeric        |
|                      | | flags in the form of a string. CSM does not allow special           |
|                      | | characters, as these represent a potential exposure, allowing       |
|                      | | unwanted activity to occur.                                         |
+----------------------+-----------------------------------------------------------------------+
| --user_flags         | | The script must have a command line parameter –user_flags. This     |
|                      | | parameter should take a space delimited list of alphanumeric        |
|                      | | flags in the form of a string. CSM does not allow special           |
|                      | | characters, as these represent a potential exposure, allowing       |
|                      | | unwanted activity to occur.                                         |
+----------------------+-----------------------------------------------------------------------+
| Returns 0 on success | | Any other error code will be captured by create/delete and the      |
|                      | | api call will fail.                                                 |
+----------------------+-----------------------------------------------------------------------+

Optional prolog/epilog Features
-------------------------------


+----------------------+-------------------------------------------------------------------------+
| Feature              | Description                                                             |
+======================+=========================================================================+
| logging              | | If the sysadmin wants to track these scripts in BDS, a form of        |
|                      | | logging must be implemented by the admin writing the script. The      |
|                      | | sample scripts outline a technique using python and the logging       |
|                      | | module.                                                               |
+----------------------+-------------------------------------------------------------------------+

Prolog/epilog Environment Variables
-----------------------------------

:CSM_ALLOCATION_ID: 
    :Allocation: Yes
    
    :Step: Yes

    The Allocation ID of the invoking CSM handler.

:CSM_SECONDARY_JOB_ID:
    :Allocation: Yes
    
    :Step: No

    The Primary Job (Batch) ID of the invoking CSM handler.

:CSM_SECONDARY_JOB_ID:
    :Allocation: Yes

    :Step: No

    The Secondary Job (Batch) ID of the invoking CSM handler.

:CSM_USER_NAME:
    :Allocation: Yes

    :Step: No

    The user associated with the job. 

.. note:: 
    A *step prolog* or *step epilog* differs in two ways: the *–type flag* is set to *step* and 
    certain environment variables will not be present. 



