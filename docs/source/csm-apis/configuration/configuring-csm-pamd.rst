.. _CSMPAM:

CSM Pam Daemon Module
=====================

The ``libcsmpam.so`` module is installed by the |csm-core| rpm to ``/usr/lib64/security/libcsmpam.so``.

To enable the this module for sshd perform the following steps:

1. Uncomment the following lines in ``/etc/pam.d/sshd``

    .. code-block:: none

        #account    required     libcsmpam.so   
        #session    required     libcsmpam.so

    .. note:: 

        The session ``libcsmpam.so`` module is deliberately configured to be the last session in this file. 
        
        If the configuration changes this make sure the ``libcsmpam.so`` is loaded after the default 
        session modules. It is recommended that ``libcsmpam.so`` be immediately after the default 
        ``postlogin`` line in the sshd config if the admin is adding additional session modules.

        The account ``libcsmpam.so`` module should be configured before the account ``password-auth``.
      
2. Run `systemctl restart  sshd.service` to restart the sshd daemon with the new config.

    After the daemon has been restarted the modified pam sshd configuration should now be used.

Contents
--------

.. contents::
   :local:

Module Behavior
---------------

This module is designed for account authentication and cgroup session assignment in the pam sshd utility.
The following checks are performed to verify that the user is allowed to access the system:

1. The user is root.

    * Allow entry.
    * Place the user in the default cgroup (session only).
    * Exit module with success.

2. The user is defined in `/etc/pam.d/csm/activelist`.

    * Allow entry.
    * Place the session in the cgroup that the user is associated with in the `activelist` (session only).
    * *note:* The `activelist` is modified by csm, admins should not modify.
    * Exit module with success.

3. The user is defined in `/etc/pam.d/csm/whitelist`.
    
    * Allow entry.
    * Place the user in the default cgroup (session only).
    * *note:* The `whitelist` is modified by the admin.
    * Exit module with success.

4. The user was not found.

    * Exit the module, rejecting the user.
    

Module Configuration
--------------------

Configuration may occur in either a pam configuration file (e.g. ``/etc/pam.d/sshd``) or the
csm pam `whitelist`.

libcsmpam.so
^^^^^^^^^^^^

:File Location: ``/usr/lib64/security/libcsmpam.so``
:Configurable:  Through pam configuration file.   

The ``libcsmpam.so`` is a session pam module. For details on configuring this module and other
pam modules please consult the linux man page (``man pam.conf``).

When |csm-core| is uninstalled, this library is always removed.

.. warning::
    The ``libcsmpam.so`` module is recommended be the last session line in the default pam 
    configuration file. The module requires the session to be established to move the session 
    to the correct cgroup. If the module is invoked too early in the configuration, users will 
    not be placed in the correct cgroup. Depending on your configuration this advice may or 
    man not be useful.

whitelist
^^^^^^^^^

:File location: ``/etc/pam.d/csm/whitelist`` 
:Configurable: Yes                        

The `whitelist` is a newline delimited list of user names. If a user is specified they will
always be allowed to login to the node. 

If the user has an active allocation on the node an attempt will be made to place them 
in the correct allocation cgroup. Otherwise, the use will be placed in the default cgroup.

When |csm-core| is uninstalled, if this file has been modified it will **NOT** be deleted.

The following configuration will add three users who will always be allowed to start a session.
If the user has an active allocation they will be placed into the appropriate cgroup as
described above.

.. code-block:: none

    jdunham
    pmix
    csm_admin


activelist
^^^^^^^^^^

:File location: ``/etc/pam.d/csm/activelist``
:Configurable:  No                         

The `activelist` file should not be modified by the admin or user. CSM will modify this file
when an allocation is created or deleted.

The file contains a newline delimited list of entries with the following format: 
``[user_name];[allocation_id]``. This format is parsed by ``libcsmpam.so`` to determine
whether or not a user can begin the session (`username`) and which cgroup it belongs 
to (`allocation_id`).

When |csm-core| is uninstalled, this file is always removed.

Module Compilation 
------------------

.. attention:: 
   Ignore this section if the csm pam module is being installed by rpm.

In order to compile this module the ``pam-devel`` package is required to compile.

Troubleshooting 
---------------

Core Isolation
^^^^^^^^^^^^^^

If users are having problems with core isolation, unable to log onto the node, or not being placed into the correct cgroup, first perform the following steps.

1. Manually create an allocation on a node that has the PAM module configured. 
    
    This should be executed from the launch node as a non root user.

    .. code-block:: bash 

        $ csm_allocation_create -j 1 -n <node_name> --cgroup_type 2
        ---
        allocation_id: <allocation_id>
        num_nodes: 1
        - compute_nodes:  <node_name>
        user_name: root
        user_id: 0
        state: running
        type: user managed
        job_submit_time: 2018-01-04 09:01:17
        ...

    **POSSIBLE FAILURES**
    
    * The allocation create fails, ensure the node is in service:

    .. code-block:: bash

        $ csm_node_attributes_update -s "IN_SERVICE" -n <node_name>
    
2. After the allocation has been created with core isolation ssh to the node ``<node_name>`` as the user who created the allocation:

    .. code-block:: bash

        $ ssh <node_name>

    **POSSIBLE FAILURES**
    
    * The `/etc/pam.d/csm/activelist` was not populated with `<user_name>`.
      
      * Verify the allocation is currently active: 
         ``csm_allocation_query_active_all | grep "allocation_id.* <allocation_id>$"``

          If the allocation is not currently active attempt to recreate the allocation.
          
      * Login to <node_name> as root and check to see if the user is on the activelist:
         
         .. code-block:: bash

            $ ssh <node_name> -l root "grep <user_name> /etc/pam.d/csm/activelist"

        If the user is not present and the allocation create is functioning this may be a CSM bug, 
        open a defect to the CSM team.
         
3. Check the cgroup of the user's ssh session.

    .. code-block:: bash

        $ cat /proc/self/cgroup
        11:blkio:/
        10:memory:/allocation_<allocation_id>
        9:hugetlb:/
        8:devices:/allocation_<allocation_id>
        7:freezer:/
        6:cpuset:/allocation_<allocation_id>
        5:net_prio,net_cls:/
        4:perf_event:/
        3:cpuacct,cpu:/allocation_<allocation_id>
        2:pids:/
        1:name=systemd:/user.slice/user-9999137.slice/session-3957.scope

    Above is an example of a properly configured cgroup. The user should be in an allocation cgroup for 
    the `memory`, `devices`, `cpuacct` and `cpuset` groups.
    
    **POSSIBLE FAILURES**

    * The user is only in the `cpuset:/csm_system` cgroup
      This generally indicates that the `libcsmpam.so` module was not added in the correct location 
      or is disabled. 
      
      Refer to the quick start at the top of this document for more details.
      
    * The user is in the `cpuset:/` cgroup.
      Indicates that core isolation was not performed, verify core isolation is enabled in the 
      allocation create step.
      
4. Any further issues are beyond the scope of this troubleshooting document, contacting the CSM team or opening a new issue is the recommended course of action.
    
Users Without Access Being Given Access
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If a user who doesn't have access is capable of logging into a node configured with the pam
library perform the following steps:

1. Verify that the following lines are uncommented in ``/etc/pam.d/sshd``
    
    .. code-block:: bash
        
        account    required     libcsmpam.so
        session    required     libcsmpam.so

2. Verify that ``account required libcsmpam.so`` is located above ``account include password-auth``

3. Verify that ``session required libcsmpam.so`` is located after the other ``session`` modules.

4. Verify that a "`csm_cgroup_login[.*]; User not authorized`" entry is present in ``/var/log/ibm/csm/csm_compute.log``

5. Any further issues are beyond the scope of this troubleshooting document, contacting the CSM team or opening a new issue is the recommended course of action.

