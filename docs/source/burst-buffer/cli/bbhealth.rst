
########
bbhealth
########


****
NAME
****


bbhealth - monitor bbProxy on the compute nodes


********
SYNOPSIS
********


/opt/ibm/bb/scripts/bbhealth ...


.. code-block:: perl

   or


service bbhealth start


***********
DESCRIPTION
***********


bbhealth is a utility that runs in the background and monitors the health of the bbProxy connection to its active bbServer.
If the connection were to close or fail, bbhealth will react by attempting to repeatedly alternate re-connection attempts to either its
primary or backup bbServer.  The primary and backup bbServers are designated in the burst buffer configuration file.

bbhealth will periodically poll the bbProxy connection.  This poll rate is settable via the --pollrate option.  Each query will be executed 
at the poll interval at the start of the second.


- \ **--pollrate**\ 
 
 Specifies the polling rate that bbhealth uses to query bbProxy's connection status.  The pollrate is in seconds, the default is 30.  During 
 multiple consecutive failures, the delay between polls is exponentially increased until --maxsleep is reached.
 


- \ **--maxsleep**\ 
 
 Specifies the maximum delay between polling events that bbhealth uses to query bbProxy's connection status.  --maxsleep is in seconds, the default is 3600.
 


- \ **-v**\ 
 
 Enables verbose output
 


