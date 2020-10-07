
#####
bbcmd
#####


****
NAME
****


bbcmd - perform burst buffer user commands


********
SYNOPSIS
********


bbcmd [COMMAND] ...


***********
DESCRIPTION
***********


The Burst Buffer services can be accessed from the Launch and Login nodes via the bbCmd tool.
It allows checkpoint libraries, such as SCR, to dynamically determine which file(s) to
transfer between GPFS and the compute node's local SSD and to start/stop/query transfers.
It also allows for privileged users to manage SSD logical volumes and query hardware state.

OUTPUT
======


The bbCmd tool will wait until a response is received from the service. For each command,
multiple types of data may be returned from the call. Since higher levels of software,
such as SCR, plan to provide additional tooling that may depend on responses from bbCmd, it is important to ensure that the output from bbCmd is easily machine parseable.
By default, the output from bbCmd will be returned in JSON format. The toplevel hierarchy of the JSON
output will be a target identifier sufficiently precise to identify individual job or job step
components. This allows for multiple job components (e.g. MPI ranks) to be targeted by a
single command, and yet keep the returned data separate for subsequent parsing.
If desired, bbCmd can alternatively output in XML format (--xml) or a more human-readable
pretty printed format (--pretty).
All commands will return an attribute called 'rc'. This contains the result code of the operation.
Zero indicates success and a non zero value indicates command failure.


- \ **--help**\ 
 
 Helptext display for commands and options
 


- \ **--config**\ 
 
 Path to the JSON configuration file
 


- \ **--jobid**\ 
 
 Job ID with respect to the workload manager
 


- \ **--jobstepid**\ 
 
 Job step ID with respect to the workload manager
 


- \ **--contribid**\ 
 
 (internal)  The contributor ID of the local bbcmd.
 


- \ **--target**\ 
 
 List of ranks that the command will target.  The rank number determines the target compute node.  The --target
 field can take a comma-separated list of indices.  Start/Stop ranges can also be specified.
 
 \ **Examples:**\ 
 
 --target=0,1
 --target=0,1,2,3
 --target=0-3
 --target=0,1,4-7
 --target=0-
 


- \ **--hostlist**\ 
 
 Comma-seperated list of hostnames.
 


- \ **--pretty**\ 
 
 Output in human-readable format
 


- \ **--xml**\ 
 
 Output in XML format
 




********
Commands
********


cancel
======


The cancel command takes a transfer handle and cancels the transfer.


- \ **--scope**\ 
 
 Scope of the cancel:
 
 BBSCOPETAG = Cancel impacts all transfers associated by the tag,
 
 BBSCOPETRANSFER = Cancel only impacts the local contribution
 


- \ **--handle**\ 
 
 Transfer handle to be cancelled
 



chmod
=====


The chmod command takes a path to a file and changes the file permissions.


- \ **--path**\ 
 
 Path of the file to chmod
 


- \ **--mode**\ 
 
 New chmod mode
 



chown
=====


The chown command takes a path to a file and changes the file's owner and group.


- \ **--path**\ 
 
 Path of the file to chown
 


- \ **--user**\ 
 
 Specifies the file's new owner
 


- \ **--group**\ 
 
 Specifies the file's new group
 



copy
====


The copy command takes a filelist and starts a copy on the target nodes specified by --target.
The filelist points to a tab delimited file with the following format:


- \ **--filelist**\ 
 
 Path to a file descripting the source and target paths for the transfer.  The file
 contains 1 line per source/target path pair in the following format:
 
 <source> <destination> <flags>
 


- \ **--handle**\ 
 
 Transfer handle to associate with the transfer
 



create
======


The create command takes a mount point, size, and options to create a logical volume. A file system is created on the logical volume and mounted. Requires super user credentials.


- \ **--mount**\ 
 
 Path to the mountpoint
 


- \ **--size**\ 
 
 Size is in units of megabytes unless specified in the suffix. A size suffix of B for bytes, S for sectors, M for megabytes, G for gigabytes, T for terabytes, P for petabytes or E for exabytes is optional.
 


- \ **--coptions**\ 
 
 LV creation options:  BBEXT4, BBXFS, BBFSCUSTOM1, BBFSCUSTOM2, BBFSCUSTOM3, BBFSCUSTOM4
 



getdeviceusage
==============


The getdeviceusage command takes the device index and returns NVMe specific data on the state of the SSD.


- \ **--device**\ 
 
 The NVMe device index to be queried on the compute node.
 
getfileinfo
===========

Returns active file transfers for a bbproxy daemon.  Requires super user credentials.

gethandle
=========



- \ **--contrib**\ 
 
 Comma-separated contributor list
 


- \ **--tag**\ 
 
 Tag to be used to associate the transfers
 



getstatus
=========


The getstatus command takes a transfer handle and returns details about the current status of the specified transfer.


- \ **--handle**\ 
 
 Transfer handle to be queried
 



getthrottle
===========


The getthrottle command takes a mount point and returns the goal transfer rate. The goal transfer rate refers to the bbServer rate that demand fetches or writes will be issued to the parallel file system.


- \ **--mount**\ 
 
 Mount point to be queried for the throttle rate (in bytes per second)
 



gettransferkeys
===============


The gettransferkeys command takes a transfer handle and returns all of the associated transfer keys.


- \ **--handle**\ 
 
 Transfer handle to be queried
 


- \ **--buffersize**\ 
 
 Maximum buffer size to retrieve
 



gettransfers
============


The gettransfers command takes a comma separated list of transfer statuses and returns all the transfer handles that currently have a status in the list. Only transfer handles associated with the job will be returned.


- \ **--matchstatus**\ 
 
 Match status values:
 BBNOTSTARTED = Transfer not started,
 BBINPROGRESS = Transfer in-progress,
 BBPARTIALSUCCESS = Partially successful transfer,
 BBFULLSUCCESS = Successful transfer,
 BBCANCELED = Canceled transfer,
 BBFAILED = Failed transfer,
 BBSTOPPED = Stopped transfer,
 BBALL = All transfers
 


- \ **--numhandles**\ 
 
 Number of handles to retrieve
 



getusage
========


The getusage command takes a mount point and returns the current statistics of I/O activity performed to that mount point.


- \ **--mount**\ 
 
 Mount point to query for usage statistics
 



mkdir
=====


The mkdir command takes a pathname and creates it.


- \ **--path**\ 
 
 Path to create on the compute node
 



remove
======


The remove command takes a mount point and removes the logical volume. This returns the associated storage for the logical volume back to the burst buffer volume group.
Requires super user credentials.


- \ **--mount**\ 
 
 Mount point to remove
 



removejobinfo
=============


The removejobinfo command removes the metadata associated with a job from the bbServers.
Requires super user credentials.


resize
======


The resize command takes a mount point, new size, and options to resize a logical volume and its file system.
Requires super user credentials.


- \ **--mount**\ 
 
 Mount point to resize
 


- \ **--size**\ 
 
 Size is in units of megabytes unless specified in the suffix. A size suffix of B for bytes, S for sectors, K for kilobytes, M for megabytes, G for gigabytes, T for terabytes, P for petabytes or E for exabytes is optional. A leading '-' or '+' sign makes the resize operation relative to the current size and not absolute.
 


- \ **--roptions**\ 
 
 The parameter is optional.  If not specified, roptions will default to BB_NONE.
 
 BB_NONE
 BB_DO_NOT_PRESERVE_FS
 



resizelglvol
============


The resizelglvol command takes a logical volume name and new size to resize a logical volume. It can be used to further shrink a logical volume whose file system was unmounted and not preserved by a previous resize command.

Requires super user credentials.


- \ **--lglvol**\ 
 
 LV logical volume name.
 


- \ **--size**\ 
 
 Size is in units of megabytes unless specified in the suffix. A size suffix of B for bytes, S for sectors, K for kilobytes, M for megabytes, G for gigabytes, T for terabytes, P for petabytes or E for exabytes is optional. A leading '-' or '+' sign makes the resize operation relative to the current size and not absolute.
 



rmdir
=====


The rmdir command takes a pathname and removes it.


- \ **--path**\ 
 
 Path to remove directory
 



setthrottle
===========


The setthrottle command takes a transfer handle and sets the goal transfer rate. The goal transfer rate refers to the bbServer rate that demand fetches or writes will be issued to the parallel file system.


- \ **--mount**\ 
 
 Mount point to be modified
 


- \ **--rate**\ 
 
 Maximum transfer rate (in bytes per second) for the mount point
 



setusagelimit
=============


The setusagelimit command takes a mount point and read and/or write limits and monitors the SSD. If the activity exceeds the set limit, a RAS event will be generated. Requires super user credentials.


- \ **--mount**\ 
 
 Mount point to monitor
 


- \ **--rl**\ 
 
 The read limit
 


- \ **--wl**\ 
 
 The write limit
 



sleep
=====


Performs a sleep operation on the compute node


- \ **--delay**\ 
 
 Delay in seconds
 



