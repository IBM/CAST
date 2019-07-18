
#########
setServer
#########


****
NAME
****


setServer - modifies the current active bbServer used by bbProxy


********
SYNOPSIS
********


/opt/ibm/bb/scripts/setServer


***********
DESCRIPTION
***********


The burst buffer setServer utility provides a means to change the connection between bbProxy and bbServer.  This can be
used for defining


- \ **--server**\ 
 
 This option sets the name of the server.  For non-drain, setServer will switch bbProxy to the server.  For drain, it will switch-away from the server
 if needed.
 


- \ **--[no]drain**\ 
 
 Drain will move any bbProxy that is currently using the specified 'server' to switch away onto its backup (or return to primary, if backup is already active)
 


- \ **-v**\ 
 
 Enables verbose output
 


