Hadoop Configuration
********************

.. note:: In the following documentation examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.

**Contents:**

.. contents::
   :local:

Quick Configuration
-------------------

As installing hadoop requires a fair amount of work to be set up, the
preceeding scripts have been included to speed up the process. 
Generally their execution will be as follows.

1. `/opt/ibm/csm/bigdata/Hadoop/hadoop_prereqs.sh`

    This script will ensure that the node has been made ready for a hadoop install.

2. `/opt/ibm/csm/bigdata/Hadoop/init_disks.sh` 
    
    This script will intializes the disks on a Habanero system for hadoop.

3. `/opt/ibm/csm/bigdata/Hadoop/iop_install.sh`
    
    This script will install iop, the configuration is a GUI process.

4. `/opt/ibm/csm/bigdata/Hadoop/hive_integration.sh`
    
    This script will automate hive integration with Log Analysis, execute from the LA node as the non root (ioala user). This script requires modifications, see  :ref: `hive-integration`


.. note:: For details about these scripts please consult their -h help output.

.. _hive-integration:

Hive Integration Script
^^^^^^^^^^^^^^^^^^^^^^^

There are 2 variables that need to be changed in `hive_integration.sh`:

:la_server: The server that hosts Log Analysis, this is used to create the hdfs directory.
:nn_server: The name node server that has an instance of HADOOP running.

Optionally if the `ioala` user is not being used for Log Analysis the `user_name` filed must be changed to match.

These variables may also be changed through command line arguments, please refer to -h for more details.

