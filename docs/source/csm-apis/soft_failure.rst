.. _csm_soft_failure_recovery:

Soft Failure Recovery
=====================

CSM defines a set of mechanisms for recovering from `Soft Failure` events. 

A `Soft Failure` is an event which is considered to be largely intermittent. Generally, a soft failure
may be caused by a networking issue or a failure in the Prolog/Epilog. CSM has a set of conditions
for which it will trigger a `Soft Failure` to prevent scheduling until the intermitten failure is complete.
It is also expected that system administrators will define `Soft Failure` events in their Prolog/Epilog.

When a node is placed into `Soft Failure` it must be returned to `In Service` before the scheduler
will be allowed to select the node for further allocations. If the node exceeds a user specified
retry count (either via recurring task or commandline) the node will be moved from `Soft Failure`
to `Hard Failure`. 

Success for moving from `Soft Failure` to `In Service` is determined by three metrics:

1. CSM is able to clear all CGroups (soft failure means the node should host no allocations).
2. The admin defined `Recovery Script`_ executed and returned zero.
3. The recovery process didn't timeout.


The following diagram is a high level abstraction of the state machine interacted with by 
the soft failure recovery mechanism:

.. graphviz::

    digraph G {
        "Soft Failure" -> "Soft Failure" [label="  Retry"];
        "Soft Failure" -> "In Service"   [labelfontcolor="#009900" label="Recovery\nSuccess" color="#009900"];
        "Soft Failure" -> "Hard Failure" [label=" Recovery\nFailure" color="#993300"];
        "In Service"   -> "Soft Failure" [label="Intermittent\nError" color="#993300"];
    }


.. contents::
    :local:


Recurring Task Configuration
----------------------------

To configure the `Soft Failure` recovery mechanism, please refer to the  
:ref:`csm_soft_failure_recovery-config` documentation.

Additionally, depending on the complexity of the `Recovery Script`_, the admin should modify 
the :ref:`CSMDAPIConf` timeout time of `csm_soft_failure_recovery` to account for at least 
twice the projected runtime of the recovery script.

Command Line Interface
----------------------

CSM provides a command line script to trigger a `Soft Failure` recovery. Invocation is as follows:

.. code-block:: bash

    /opt/ibm/csm/bin/csm_soft_failure_recovery -r <retry_threshold>

The `-r` or `--retry` option sets a retry threshold if this threshold is exceeded or met by any nodes
that failed to be placed into `In Service` the node will be moved to `Hard Failure`.

.. attention:: Nodes that are in `Soft Failure` and owned by an allocation will *NOT* be processed by this
    utility!

Recovery Script
---------------

.. attention:: A recovery script must be located at `/opt/ibm/csm/recovery/soft_failure_recovery`
    to use the `Soft Failure` recovery mechanism!

A sample of the recovery script is placed in `/opt/ibm/csm/share/recovery` when installing the 
`ibm-csm-core` rpm. The sample script is extremely basic and is expected to be modified by the end
user. 

A recovery script must fit the following criteria:

1. Be located at `/opt/ibm/csm/recovery/soft_failure_recovery`.
2. Return `0` if the recovery was a success.
3. Return `> 0` in the event the recovery failed. 

The recovery script takes no input parameters at this time.

