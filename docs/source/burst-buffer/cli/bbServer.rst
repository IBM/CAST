
########
bbServer
########


****
NAME
****


bbServer - burst buffer server process for the I/O nodes


********
SYNOPSIS
********


bbServer [--help] [--whoami=string] [--instance=value] [--config=path]


***********
DESCRIPTION
***********


The bbServer is a persistent process running on each of the ESS I/O nodes. The role of the
bbServer is to push or pull traffic from the SSDs, track status of transfers, and to
handle requests from the bbProxy or other bbServers.


- \ **--help**\ 
 
 Display the help text
 


- \ **--whoami**\ 
 
 Identifies the name of the bbServer configuration.
 


- \ **--instance**\ 
 
 Unused
 


- \ **--config**\ 
 
 Path to the JSON configuration file.
 


