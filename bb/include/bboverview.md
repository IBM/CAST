# Burst Buffer Overview {#bboverview}

## Stage-in

A stage-in script is started by LSF on a FEN.  At this point, the compute node(s) have not been allocated by CSM, although the specific compute nodes that will be assigned are known by LSF and the stage-in script.

This stage-in script has 2 phases:
- Administration stage-in script
- User stage-in script

The administration stage-in script is executed.  A primary function of this script is to create an LVM logical volume that will be used as storage for stage-in activities.  Once the administration script has finished its tasks, it switches to the user context (as known by LSF) and calls the user-specified stage-in script.

 Both the administration and user stage-in scripts use the bbCmd executable to communicate with the compute nodes.

### Note
Raw BBAPI commands are not available because the compute node(s) have not been allocated, so running user-code on the compute node(s) is not permitted.  This permission is denied because it might interfere with other user jobs.


## Runtime
Once the hardware has been allocated, LSF launches the runtime script.  This script contains various jsrun commands to start user tasks on the compute nodes.

The user could continue to use bbcmd commands to stage-in/stage-out files during runtime.  But they will also be able to use the \ref bbapi directly on the compute node to control the file transfers.


## Stage-out

 A stage-out script is started by LSF on a FEN when the runtime script exits.  At this point, the compute node(s) has been deallocated by CSM, the logical volume on the SSD will still exist.

 This stage-out script has 3 phases:
 - Administration stage-out (prolog)
 - User stage-out
 - Administration stage-out (epilog)

The administration stage-out script is executed, and immediately switches to the user context (as known by LSF) and calls the user-specified stage-out script.  Once the user-script has completed its tasks and exits, then the administration stage-out resumes control and performs various bookkeeping tasks:
 - Collect SSD usage statistics
 - Remove the SSD logical volume
 - Update CSM SSD logical volume table

 Both the administration and user stage-in scripts use the bbCmd executable to communicate with the compute nodes.

### Note
Raw BBAPI commands are not available because the compute node(s) have not been allocated, so running user-code on the compute node(s) is not permitted.  This permission is denied because it might interfere with other user jobs.


## Transfers

 When a transfer is started, the user specifies:
 - A unique numeric identifier for the transfer, called a tag.
 - A list of other processes contributing data for the tag.
 - A list of source and destination paths for the files to transfer.

 Once the transfer has started, the burst buffer software will provide a handle for the transfer.


## Tag Matching

- bbServer receives the transfer request from the process.  Transfer request contains the tag and list of contributors
- bbServer looks for transfers that may have been started that match the tag# *and* identify the requesting process as a contributor.
- If no transfers have been identified, bbServer creates a new transfer bundle
- Otherwise, it marks the new contributor as started in the existing transfer bundle

 All bbServers will need to be notified of the transfer request.  However, they do not need to all know the physical blocks.


## Passing Credentials between BBAPI and bbCmd

 The user's UID and GID of the bbcmd are transfered down to the compute node.  When the command is executed on the compute node, it is performed under that UID and GID.


\par Filename Pattern substitutions

 Filenames can contain substitution text.  The intention is that stage-in / stage-out scripts can specify a pattern, which is expanded on the compute node.  This allows for CSMI to send a single request down the communication tree to all compute nodes.

 The list of accepted patterns:

 \verbatim
 %hostname%
 %index%
 \endverbatim
