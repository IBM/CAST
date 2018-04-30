# CSM Prolog/Epilog Documentation #

The **privileged_prolog**, **privileged_epilog** and **privileged.ini** 
files demonstrate the framework for a privileged prolog/epilog.

This sample code is written in python, but any language is supported
provided the executable/scripts match the following requirements:

* The scripts are present in **/opt/ibm/csm/prologs/** with the following names:
    * **privileged_prolog**
    * **privileged_epilog**
* The scripts must accept the following values:
    * --type [allocation|step]
    * --user_flags "[string of flags, spaces allowed]" (Alphanumeric flags)
    * --sys_flags "[string of flags, spaces allowed]"  (Alphanumeric flags)
* The scripts must return zero for a successful execution and non zero for a failed execution.

The sample post scripts are shipped to **/opt/ibm/csm/share/prologs/** in the **ibm-csm-core** rpm.
Before an allocation may be created these scripts, or user defined scripts must be copied to 
**/opt/ibm/csm/prologs/** on each compute node. A sample of how to achieve this is as follows:

```
    mkdir -p /opt/ibm/csm/prologs/; 
    cp -p /opt/ibm/csm/share/prologs/ /opt/ibm/csm/prologs;
```    
**NOTE:** At the time of writing `user_flags` and `sys_flags` only support alphanumeric characters for security purposes.

**WARNING:** By default stdout and stderr are **NOT** captured by CSM, it is recommended that a logging utility
    be used for important messages and tracking. The sample code shows the use of the python logging module.

## Environment Variables ##

When these scripts are invoked by CSM they are guaranteed to have the following environment variables set. If an environment variable is not supported for the prolog/epilog type the contents of Environment Variable is currently undefined.

**WARNING:** This list is subject to change.

| Environment Variable | Description                                              | Allocation Support | Step Support |
|----------------------|----------------------------------------------------------|--------------------|--------------|
| CSM_ALLOCATION_ID    | The Allocation ID of the invoking CSM handler.           | Yes                | Yes          |
| CSM_PRIMARY_JOB_ID   | The Primary Job(Batch) ID of the invoking CSM handler.   | Yes                | No           |
| CSM_SECONDARY_JOB_ID | The Secondary Job(Batch) ID of the invoking CSM handler. | Yes                | No           |
| CSM_USER_NAME        | The user associated with the job                         | Yes                | No           |

## Shipped Files ##

CSM ships four files including this README for prolog/epilog scripts. It is not mandatory that
these files be used for prologs/epilogs, they are merely sample implementations. It is expected
that the system administrator replace the contents of these scripts to fit their environment and
cluster requirements.

| File              | Description                    | Access Level |
|-------------------|--------------------------------|--------------|
| privileged_prolog | A sample python prolog script. | 700          |
| privileged_epilog | A sample python epilog script. | 700          |
| privileged.ini    | A sample python logger config. | 600          |
| POST_README       | This file.                     | 444          |
