Burst Buffer Installation
=========================


Preface
-------
The document has been updated to reflect installing into Red Hat Enterprise Linux v8.1.

After the general outline of the install is a discussion of using supplied ansible playbooks to
handle install, starting, stopping, and uninstalling  burst buffer across the involved computing nodes.

Pre-requisites
--------------

Red Hat Enterprise Linux v8.1 or later for POWER9 installed on the nodes.

* ibm-burstbuffer-1.x.y-z.ppc64le.rpm
* ibm-burstbuffer-lsf-1.x.y-z.ppc64le.rpm
* ibm-burstbuffer-mn-1.x.y-z.ppc64le.rpm
* ibm-csm-core-1.x.y-z.ppc64le.rpm
* ibm-export_layout-1.x.y-z.ppc64le.rpm
* ibm-flightlog-1.x.y-z.ppc64le.rpm

Security Certificates
---------------------

The connection between bbProxy and bbServer is secured via a X.509 certificate.  To create a certificate, the openssl tool can be used.  For convenience, a provided bash script can be used:
/opt/ibm/bb/scripts/mkcertificate.sh

This command will generate two files and only needs to be run on a single node:

.. code-block:: none

    -rw-r--r-- 1 root root cert.pem
    -rw------- 1 root root key.pem


The key.pem file is the private key and should be kept secret.  We recommend copying this same file to /etc/ibm/key.pem on each of the bbServer nodes.  

The cert.pem file should be copied to all bbServer and compute nodes.  The cert.pem can be deployed to the compute nodes in a variety of ways.  For example,

* cert.pem could be placed in shared GPFS storage.  
* cert.pem could be placed in the xCAT compute image.
* cert.pem could be rsync’d/scp’d to each compute node after boot.


ESS I/O node VM setup
---------------------

RPM installation
****************

On each ESS I/O Node VM (or equivalent), install these RPMs:	

* ibm-burstbuffer-1.x.y-z.ppc64le.rpm 
* ibm-flightlog-1.x.y-z.ppc64le.rpm 
* ibm-csm-core-1.x.y-z.ppc64le.rpm


The NVMe driver must be built with RDMA-enabled.  Mellanox MOFED driver can do this via:

.. code-block:: bash

	%  mlnxofedinstall --with-nvmf

Security Files
**************

The cert.pem and key.pem files should ideally be placed in the /etc/ibm directory on each of the bbServer nodes.  This can be done after the VM has been booted or during image creation.


Starting BB Services
********************

The burst buffer server can be started through the following command issued on each ESS I/O node VM (or equivalent):

.. code-block:: bash

	/opt/ibm/bb/scripts/bbactivate --server

This command will use the default BB configuration file in the RPM (unless overridden by --config) and start the burst buffer server.  It will also add the NVMe over Fabrics block device pattern to the global_filter in /etc/lvm/lvm.conf (unless the global filter line has already been modified)


Compute Node setup
------------------ 

RPM installation
****************

On each Compute Node, install these RPMs:

* ibm-burstbuffer-1.x.y-z.ppc64le.rpm 
* ibm-flightlog-1.x.y-z.ppc64le.rpm 
* ibm-export_layout-1.x.y-z.ppc64le.rpm 
* ibm-csm-core-1.x.y-z.ppc64le.rpm

The NVMe driver must be built with RDMA-enabled.  Mellanox MOFED driver can do this via:

.. code-block:: bash

	%  mlnxofedinstall --with-nvmf


Security Files
**************

The cert.pem file should ideally be placed in the /etc/ibm directory on each of the compute nodes.  This can be done after the node has been booted or during image creation.  The private key (key.pem) should not be placed on the compute node.


Compute Node and ESS list generation
************************************

The burst buffer has a static assignment of compute nodes to bbServers.  This relationship  is defined by two files that are specified via the bbactivate tool.  

The first file (nodelist) is a list of the xCAT names for all compute nodes – one compute node per line.  E.g.,:

.. code-block:: bash

	c650f07p23
	c650f07p25
	c650f07p27


This nodelist could be generated via the xCAT commands:
lsdef all | grep "Object name:" | cut -f 3 -d ' '


The second file (esslist) contains a list of IP addresses and ports for each bbServer.  
In the planned configuration, this would be the ESS I/O node VM IPv4 address plus a well-known port (e.g., 9001).  
To express ESS redundancy, after "IPv4:port" add "backup=IPv4:port".  For example:

.. code-block:: none

20.7.5.100:9001 backup=20.7.5.101:9001
20.7.5.101:9001 backup=20.7.5.100:9001


Starting BB Services
********************

On each compute node, run the bbactivate tool:

.. code-block:: bash

	$ /opt/ibm/bb/scripts/bbactivate


Running the bbServer on a different node than bbProxy requires a networked block device to be configured.  If no block device is configured, the bbactivate script will attempt to establish  an NVMe over Fabrics connection between the two nodes when bbProxy is started.  

Whenever a compute node is rebooted or SSD is replaced, rerun the bbactivate tool.  


Launch/Login Node setup
-----------------------

RPM installation
****************

On each Launch/Login Node, install these RPMs:
* ibm-burstbuffer-1.x.y-z.ppc64le.rpm
* ibm-flightlog-1.x.y-z.ppc64le.rpm 
* ibm-csm-core-1.x.y-z.ppc64le.rpm
* ibm-burstbuffer-lsf-1.x.y-z.ppc64le.rpm


The burstbuffer-lsf RPM also permits relocation::

    $ rpm --relocate /opt/ibm/bb/scripts=$LSF_SERVERDIR …


LSF Setup
*********

Further LSF configuration should be performed to setup the data transfer queues.  Please refer to the LSF installation documents for details.  
https://www.ibm.com/support/knowledgecenter/en/SSWRJV_10.1.0/lsf_csm/lsf_csm_burst_buffer_config.html

It is also recommended to add the following parameter to the lsf.conf file so that the burst buffer esub.bb and epsub.bb scripts are executed on job submission to setup key environment variables for $BBPATH and BSCFS:
LSB_ESUB_METHOD=bb


BB Configuration
****************

A directory is used to store job-specific bscfs metadata between job execution and job stage-out.  Create a path in parallel file system for bscfs temporary files.  The workpath should be accessible to all users.  

A path is also needed to specify temporary storage for job-related metadata between the job submission through job stageout.  It must be a location that can be written by the user and read by root, and accessible by nodes used for job submission and launch.  It does not need to be accessible by the compute nodes.  If the user home directories are readable by root, --envdir=HOME can be used.  

For LSF configuration, several scripts need to be copied into $LSF_SERVERDIR.  The files that need to be copied from /opt/ibm/bb/scripts are:  esub.bb, epsub.bb, esub.bscfs, epsub.bscfs, bb_pre_exec.sh, and bb_post_exec.sh.  The bbactivate script can automatically copy these files.  Alternatively, the burstbuffer-lsf RPM is relocatable.

$. /opt/ibm/bb/scripts/bbactivate --ln --bscfswork=$BSCFSWORK --envdir=HOME --lsfdir=$LSF_SERVERDIR


Management Node setup (optional)
--------------------------------

RPM installation
****************

On the CSM Management Node, install this RPM:	
* ibm-burstbuffer-mn-1.x.y-z.ppc64le.rpm

Adding burst buffer RAS into CSM Database
*****************************************

RAS definitions for the Burst Buffer can be added to CSM postgres tables via the following command:

.. code-block:: bash

	$  /opt/ibm/csm/db/csm_db_ras_type_script.sh -l csmdb /opt/ibm/bb/scripts/bbras.csv

This command should be executed on the CSM management node.  The ibm-burstbuffer-mn RPM must also be installed on the management node.  

If the RAS definitions are not added, the bbProxy log will show errors posting any RAS messages; however the errors are benign.  



Stopping the Services
---------------------

Stopping the burst buffer processes can be done via:

.. code-block:: bash

	$ /opt/ibm/bb/scripts/bbactivate --shutdown

To teardown all NVMe over Fabrics connections, from each I/O Node use:

.. code-block:: bash

	$ nvme disconnect –n burstbuffer


Using BB Administrator Failover
-------------------------------

There may be times in which the node running bbServer needs to be taken down for scheduled maintenance.  The burst buffer provides a mechanism to dynamically change and migrate transfers to a backup bbServer.  The backup bbServer is defined in the configuration file under backupcfg.  

To switch to the backup server on 3 compute nodes cn1,cn2,cn3:

.. code-block:: bash

	xdsh cn1,cn2,cn3 /opt/ibm/bb/scripts/setServer –server=backup

To switch back to the primary server on 3 compute nodes cn1,cn2,cn3:

.. code-block:: bash

	xdsh cn1,cn2,cn3 /opt/ibm/bb/scripts/setServer –server=primary


If submitting the switchover via an LSF job that runs as root, the –hosts parameter can be removed as setServer will use the compute nodes assigned by LSF.  


Optional Configurations
-----------------------

Single node loopback (optional)
*******************************

The bbProxy and bbServer can run on the same node, although this is development/bringup configuration (e.g., a bbAPI-using application development).  In the ESS I/O node list would contain a line specifying loopback address (127.0.0.1:9001) for each compute node.  Both lists need to have the same number of lines.  


Configuring bbProxy without CSM (optional)
******************************************

bbProxy can update CSM on the state of the logical volumes and emit RAS via CSM interfaces.  This is automatically configured via the bbactivate tool.

.. code-block:: bash

	/opt/ibm/bb/scripts/bbactivate --csm
	/opt/ibm/bb/scripts/bbactivate --nocsm

The default is to enable CSM


Configuring without Health Monitor (optional)
*********************************************

The burst buffer has an external process that can monitor the bbProxy->bbServer connection.  If the connection becomes offline, the health monitor will either attempt to re-establish the connection, or (if defined) establish a connection with the backup server.

By default, bbactivate will start the burst buffer health monitor.  This behavior can be changed via the --nohealth option to bbactivate:

.. code-block:: bash

	/opt/ibm/bb/scripts/bbactivate --nohealth


Ansible playbooks for burstbuffer
*********************************

Install ibm-burstbuffer-ansible RPM on the machine where ansible-playbook will be run.  The localhost needs to have connections 
to all the nodes involved in the cluster.  
Copy all the CAST RPMs into a directory in a parallel file system with the same mount across all the nodes in the cluster.

Inventory
---------  

Need an ansible inventory of hosts naming nodes by grouping: 
compute, where bbproxy daemon will reside with a local nvme drive and applications run;
server, where bbserver daemon will run and conduct transfers between the compute nvme drive and GPFS;
launch, where lsf jobs will be submitted and communication will take place with the compute node bbproxy daemons; and
management, where management csm daemons reside.

An example inventory file:
[compute]
c650f06p25
c650f06p27 
c650f06p29

[server]
gssio1vm-hs backup=gssio2vm-hs
gssio2vm-hs backup=gssio1vm-hs

[management]
c650mnp03

[launch]
c650mnp03
<EOF>

Install by ansible-playbook
---------------------------
Advice is to do these in order:

export RPMPATH=/gpfs/CAST/RPM
export Inventory=/root/hosts
export KEYFILE=/root/key.pem
export CERTFILE=/root/cert.pem
sudo ansible-playbook -f 16 -i $Inventory -e BBRPMDIR=$RPMPATH -e CSMRPMDIR=$RPMPATH  /opt/ibm/bb/ansible/nodelist.yml
sudo ansible-playbook -f 16 -i $Inventory -e BBRPMDIR=$RPMPATH -e CSMRPMDIR=$RPMPATH  /opt/ibm/bb/ansible/bbserverIPlist.yml
sudo ansible-playbook -f 16 -i $Inventory -e BBRPMDIR=$RPMPATH -e CSMRPMDIR=$RPMPATH  /opt/ibm/bb/ansible/csmInstall.yml
sudo ansible-playbook -f 16 -i $Inventory -e BBRPMDIR=$RPMPATH -e CSMRPMDIR=$RPMPATH  /opt/ibm/bb/ansible/bbInstall.yml
sudo ansible-playbook -f 16 -i $Inventory -e FQP_KEYFILE=$KEYFILE -e FQP_CERTFILE=$CERTFILE  /opt/ibm/bb/ansible/certificates.yml

Activation by ansible-playbook
------------------------------
Advice is to do these in order:

sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/csmStart.yml
sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/bbStart.yml

Stop by ansible-playbook
------------------------
Advice is to do these in order:

sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/csmStop.yml
sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/bbStop.yml

Uninstall playbooks
-------------------
Advice is to do these in order:

sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/csmUninstall.yml
sudo ansible-playbook -f 16 -i $Inventory   /opt/ibm/bb/ansible/bbUninstall.yml



