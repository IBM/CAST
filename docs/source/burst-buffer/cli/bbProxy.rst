
#######
bbProxy
#######


****
NAME
****


bbProxy - burst buffer proxy process for the compute nodes


********
SYNOPSIS
********


bbProxy [--help] [--whoami=string] [--instance=value] [--config=path]


***********
DESCRIPTION
***********


The bbProxy is a burst buffer component that runs on each compute node. It connects
all the programs running on the compute node using the bbAPI to bbServer processes
running on the ESS I/O nodes.


- \ **--help**\ 
 
 Display the help text
 


- \ **--whoami**\ 
 
 Identifies the name of the bbProxy configuration.
 


- \ **--instance**\ 
 
 Unused
 


- \ **--config**\ 
 
 Path to the JSON configuration file.
 


