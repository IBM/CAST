Burst Buffer Commands
=====================

The Burst Buffer services can be accessed from the Front End Node (FEN) via the bbcmd tool. It allows checkpoint libraries, such as SCR, to dynamically determine which file(s) to transfer between GPFS and the compute node’s local SSD and to start/stop/query transfers. It also allows for privileged users to manage SSD logical volumes and query hardware state.

**Tools**

 .. toctree::
    :maxdepth: 1

    cli/bbcmd.rst
    cli/bbactivate.rst
    cli/bbconnstatus.rst
    cli/setServer.rst

**Daemons**

 .. toctree::
    :maxdepth: 1

    cli/bbProxy.rst
    cli/bbServer.rst
    cli/bbhealth.rst
