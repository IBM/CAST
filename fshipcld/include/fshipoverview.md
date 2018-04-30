# Function-shipping Overview {#fshipoverview}

## fshipcld daemon

The fship client daemon (fshipcld) resides on the remote node.  In an HPC environment, the fshipcld resides on the compute node.  

### Note
The fuse module must be installed so that there is a /dev/fuse device.


## fshipd
The fship daemon (fshipd) receives function ship messages from fshipcld and processes them against a target directory on its hosting system.  The fshipd sends a reply to the function-ship request to fshipd.

##fshipmond
The fship monitor daemon (fshipmond) launches an fshipd after receiving a message from fshipcld.
The fshipmond monitors its children processes and logs when those processes end.



