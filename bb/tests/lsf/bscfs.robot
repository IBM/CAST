*** Settings ***
Resource        ../common.robot
Test Setup      Setup LSF testcases
Suite setup     Clear LSF jobs
Suite teardown  Clear LSF jobs

*** Keywords ***
bscfs cleanup
	 :FOR  ${i}   in range   0   10
	 \  Run Keyword And Continue On Failure  remove files  ${PFSDIR}/chkpnt_00${i}  ${PFSDIR}/chkpnt_00${i}.mapfile
	 \  Run Keyword And Continue On Failure  remove files  ${PFSDIR}/chkpnt_01${i}  ${PFSDIR}/chkpnt_01${i}.mapfile

*** Test Cases ***

bscfs access check
      [Tags]  lsf  bscfs
      [Timeout]  5 minutes
      Using SSD  16
      Using bscfs
      ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
      Set num computes  ${maxnodes}
      bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/accesscheck.sh  0  5mins

bscfs blocking write single-node
	 [Tags]  lsf  bscfs
   	 [Timeout]  15 minutes
	 Using SSD  256
	 Using bscfs
	 
	 bscfs cleanup	 
	 bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write_blocking --chkpnt_count 2 --chkpnt_size 16G --chunk_size 16M --chkpnt_dir . --keep_all  0  15mins
	 
	 bscfs cleanup

bscfs blocking write multi-node
	 [Tags]  lsf  bscfs
   	 [Timeout]  15 minutes
	 Using SSD  256
	 Using bscfs
     ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	 Set num computes  ${maxnodes}
	 
	 bscfs cleanup	 
	 bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write_blocking --chkpnt_count 2 --chkpnt_size 16G --chunk_size 16M --chkpnt_dir . --keep_all  0  15mins
	 
	 bscfs cleanup

bscfs checkpoint write series single-node
	 [Tags]  lsf  bscfs
   	 [Timeout]  15 minutes
	 Using SSD  256
	 Using bscfs
	 
	 bscfs cleanup	 
	 bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write --compute_time 40 --chkpnt_count 5 --chkpnt_size 1G --chunk_size 256M --chkpnt_dir . --keep_all  0  15mins
	 
	 bscfs cleanup


bscfs checkpoint write series multi-server
	 [Tags]  lsf  bscfs
   	 [Timeout]  30 minutes
	 Using SSD  256
	 Using bscfs
     ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	 Set num computes  ${maxnodes}
	 
	 bscfs cleanup	 
	 bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write --compute_time 40 --chkpnt_count 5 --chkpnt_size 1G --chunk_size 256M --chkpnt_dir . --keep_all  0  30mins
	 
	 bscfs cleanup


bscfs checkpoint read series single-node
      [Tags]  lsf  bscfs
      [Timeout]  30 minutes
      Using SSD  256
      Using bscfs
      Set num computes  1

      bscfs cleanup
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write --compute_time 40 --chkpnt_count 5 --header_size 4K --chkpnt_size 1G --stripe_size 512M --chkpnt_dir . --keep_all  0  30mins
      
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_read --compute_time 40 --chkpnt_count 5 --header_size 4K --chkpnt_size 1G --stripe_size 512M --chkpnt_dir .  0  30mins
      bscfs cleanup


bscfs checkpoint read series multi-server
      [Tags]  lsf  bscfs
      [Timeout]  30 minutes
      Using SSD  256
      Using bscfs
      ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	  Set num computes  ${maxnodes}

      bscfs cleanup
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_write --compute_time 40 --chkpnt_count 5 --header_size 4K --chkpnt_size 1G --stripe_size 512M --chkpnt_dir . --keep_all  0  30mins
      
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_read --compute_time 40 --chkpnt_count 5 --header_size 4K --chkpnt_size 1G --stripe_size 512M --chkpnt_dir .  0  30mins
      bscfs cleanup


bscfs checkpoint write every kth checkpoint single-node
      [Tags]  lsf  bscfs
      [Timeout]  30 minutes
      Using SSD  256
      Using bscfs
      Set num computes  1
      
      bscfs cleanup
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_combo -compute_time 40 --chkpnt_count 12 --chkpnt_start 0 --primary_interval 3 --header_size 4K --chkpnt_size 6G --stripe_size 512M --chkpnt_dir .  0  30mins
      bscfs cleanup


bscfs checkpoint write every kth checkpoint multi-server
      [Tags]  lsf  bscfs
      [Timeout]  30 minutes
      Using SSD  256
      Using bscfs
      ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	  Set num computes  ${maxnodes}
      
      bscfs cleanup
      bsub&wait  ${jsrun} ${WORKDIR}/bscfs/tests/chkpnt_combo -compute_time 40 --chkpnt_count 12 --chkpnt_start 0 --primary_interval 3 --header_size 4K --chkpnt_size 6G --stripe_size 512M --chkpnt_dir .  0  30mins
      bscfs cleanup
