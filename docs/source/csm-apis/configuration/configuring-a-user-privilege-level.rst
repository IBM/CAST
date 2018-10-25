Configuring a user’s privilege level
====================================

A user can be in one of two privileged levels, either *privileged* or *non-privileged*. To be considered a privileged user, you must either be specified as a privileged user in the ACL file or be a member of a user group that is specified in the ACL file. If a user is neither specified nor a part of a user group that was specified, then they are considered a non-privileged user. Below is a capture of the section of the ACL file that deals with setting user privilege.

.. code-block:: json
    "privileged_user_id": "root",
    "privileged_group_id": "root",

Below is an alternate example of the ACL file, if a system administrator was to alter these fields. 

.. code-block:: json
    "privileged_user_id": "jsmith",
    "privileged_group_id": "csm_admin",

The .acl file supports multiple privileged users. An example of an .acl file with multiple privileged users is shown below.

.. code-block:: json
    "privileged_user_id": [
        "root",
        "jsmith"],
    "privileged_group_id": "sys_admin",

The .acl file also supports multiple privileged user groups. An example of an .acl file with multiple privileged user groups is shown below.

.. code-block:: json
    "privileged_user_id": "root",
    "privileged_group_id":[
        "sys_admin",
        "csm_admin"],


