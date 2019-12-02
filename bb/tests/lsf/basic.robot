*** Settings ***
Resource        ../common.robot
Test Setup      Setup LSF testcases
Suite setup     Clear LSF jobs
Suite teardown  Clear LSF jobs


*** Test Cases ***

Basic setup using LSF
	[Tags]  lsf
	[Timeout]  10 minutes
	using SSD  100
	bsub&wait  hostname

Basic BSCFS setup using LSF
	[Tags]  lsf
	[Timeout]  10 minutes
	using SSD  100
	using bscfs
	bsub&wait  ${jsrun} df

Load job queue with BB
     [Tags]  lsf
     using SSD  100
     :FOR  ${i}   IN RANGE   1   10
     \  bsub  job${i}  hostname
     
     :FOR  ${i}   IN RANGE   1   10
     \  waitproc  job${i}  0

Load job queue with BSCFS
     [Tags]  lsf
     using SSD  100
     using bscfs

     :FOR  ${i}   IN RANGE   1   10
     \  bsub  job${i}  hostname
     
     :FOR  ${i}   IN RANGE   1   10
     \  waitproc  job${i}  0

Stage-in script does not exist
	[Tags]  lsf
	[Timeout]  1 minute
	Using SSD  10
	Set user stagein  this-script-does-not-exist
	bsub&wait  hostname  126

Stage-out1 script does not exist
	[Tags]  lsf
	[Timeout]  1 minute
	Using SSD  10
	Set user stageout1  this-script-does-not-exist
	${result}=  bsub&wait  ${jsrun} hostname  0

	${jobid}=  get jobid  ${result}
	Wait for stageout to complete  ${jobid}
	
	${rc2}  ${jobhist}=  Run and Return RC and Output  bread -i123 ${jobid} | tee tmp.${jobid} 
	${ignore}  ${c}=  Run and Return RC and Output  grep completed tmp.${jobid}|grep -c "rc 2"
	Log  JOBHIST: ${jobhist}
	Should be equal as integers  ${c}  1

Stage-out2 script does not exist
	[Tags]  lsf
	[Timeout]  1 minute
	Using SSD  10
	Set user stageout2  this-script-does-not-exist
	${result}=  bsub&wait  ${jsrun} hostname  0

	${jobid}=  get jobid  ${result}
	Wait for stageout to complete  ${jobid}

	${rc2}  ${jobhist}=  Run and Return RC and Output  bread -i124 ${jobid} | tee tmp.${jobid} 
	${ignore}  ${c}=  Run and Return RC and Output  grep completed tmp.${jobid}|grep -c "rc 2"
	Log  JOBHIST: ${jobhist}
	Should be equal as integers  ${c}  1 

Stage-in script is started
	 [Tags]  lsf
	 [Timeout]  1 minute
	 Using SSD  10
	 set environment variable  STAGEIN_DONE  ${PFSDIR}/done
	 Set user stagein  ${WORKDIR}/bb/tests/bin/stagein.pl
	 bsub&wait  ${jsrun} hostname
	 Sleep   10s
	 File Should Exist  %{STAGEIN_DONE}
	 Remove file  %{STAGEIN_DONE}

Stage-out1 script is started
	 [Tags]  lsf  todo
	 [Timeout]  1 minute
	 Using SSD  10
	 set environment variable  STAGEOUT1_DONE  ${PFSDIR}/done
	 Set user stageout1  ${WORKDIR}/bb/tests/bin/stageout1.pl
	 bsub&wait  hostname
	 Sleep   10s
	 File Should Exist  %{STAGEOUT1_DONE}
	 Remove file  %{STAGEOUT1_DONE}

Stage-out2 script is started
	 [Tags]  lsf  todo
	 [Timeout]  1 minute
	 Using SSD  10
	 set environment variable  STAGEOUT2_DONE  ${PFSDIR}/done
	 
	 Set user stageout2  ${WORKDIR}/bb/tests/bin/stageout2.pl
	 bsub&wait  hostname
	 Sleep   10s
	 File Should Exist  %{STAGEOUT2_DONE}
	 Remove file  %{STAGEOUT2_DONE}

LSF transfers to SSD from gpfs single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}
	
LSF transfers to SSD from gpfs multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} ${LARGEFILESIZE}

LSF transfers to SSD from devzero single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

LSF transfers to SSD from devzero multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

LSF transfers from SSD to gpfs single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}

LSF transfers from SSD to gpfs multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} ${LARGEFILESIZE}

LSF transfers from SSD to devnull single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

LSF transfers from SSD to devnull multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

LSF throttled transfers to SSD from devzero single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	set environment variable  BBTHROTTLERATE  1073741824

	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

LSF throttled transfers to SSD from devzero multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	set environment variable  BBTHROTTLERATE  1073741824

	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 /dev/zero ${LARGEFILESIZE}

LSF throttled transfers from SSD to devnull single node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	set environment variable  BBTHROTTLERATE  1073741824

	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

LSF throttled transfers from SSD to devnull multi node
	[Tags]  lsf
	[Timeout]  10 minutes
	Using SSD  512
	set environment variable  BBTHROTTLERATE  1073741824
	
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}
	
	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null ${LARGEFILESIZE}

LSF get handle create performance single node iterative scale
	[Tags]  lsf
	[Timeout]  20 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 16
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 32
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 64
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 128
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 256
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 1024
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 2048
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 4096


LSF get handle spanner single node test
	[Tags]  lsf spanner
	[Timeout]  20 minutes
	Using SSD  512
	Set num computes  1
	Set ppn  16
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_spanner_perf 512

LSF get handle spanner multiple node test
        [Tags]  lsf spanner
        [Timeout]  20 minutes
        Using SSD  512 
        ${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
        Set num computes  ${maxnodes}
        Set ppn  20  
        bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_spanner_perf 1024	

LSF get handle create performance multi node iterative scale
	[Tags]  lsf
	[Timeout]  20 minutes
	Using SSD  512
	
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 16
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 32
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 64
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 128
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 256
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 1024
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 2048
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 4096


LSF get handle create performance single node
	[Tags]  lsf
	[Timeout]  20 minutes
	Using SSD  512
	
	Set num computes  1
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  2
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  8
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  16
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512


LSF get handle create performance multi node
	[Tags]  lsf
	[Timeout]  20 minutes
	Using SSD  512
	
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  2
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  4
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  8
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  16
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_handle_perf 512

LSF 128 GiB transfer to GPFS multi node
	[Tags]  lsf
	[Timeout]  40 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} 137438953472  0  20 minutes
	
	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 ${PFSDIR} 6871947674  0  20 minutes

LSF 128 GiB transfer from GPFS multi node
	[Tags]  lsf
	[Timeout]  40 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} 137438953472  0  20 minutes
	
	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 0 ${PFSDIR} 6871947674  0  20 minutes

LSF 128 GiB transfer to devnull multi node
	[Tags]  lsf
	[Timeout]  40 minutes
	Using SSD  512
	${maxnodes} =  Run  /opt/ibm/csm/bin/csm_node_resources_query_all | grep IN_SERVICE | wc -l
	Set num computes  ${maxnodes}
	Set ppn  1
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null 137438953472  0  20 minutes
	
	Set ppn  20
	bsub&wait  ${jsrun} ${WORKDIR}/bb/tests/bin/test_basic_xfer 1 /dev/null 6871947674  0  20 minutes
