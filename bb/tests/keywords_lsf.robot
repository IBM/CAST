*** Settings ***
Library       OperatingSystem
Library       Process
Library       String
Library       Collections

*** Keywords ***
Clear LSF jobs
      Run Process  bkill  -q  bbq  0

Setup LSF Testcases
      Remove environment variable  BB_SSD_MIN  BB_STGIN_USER  BB_STGOUT1_USER  BB_STGOUT2_USER  BSCFS_WORK_PATH
      set test variable  ${application}      "bb"
      set test variable  ${BB_SSD_MIN}       0
      set test variable  ${BB_SSD_MAX}	     0
      set num computes 	  1
      set ppn		  1
      set user stagein    ${empty}
      set user stageout1  ${empty}
      set user stageout2  ${empty}
      Setup CMDLAUNCHER

using bscfs
      set test variable  ${application}  "bscfs"
      set environment variable  BSCFS_PFS_PATH  ${PFSDIR}

using SSD
    [Arguments]  ${minsize}
    set test variable  ${BB_SSD_MIN}  ${minsize}

grow SSD to
    [Arguments]  ${maxsize}
    set test variable  ${BB_SSD_MAX}  ${maxsize}
    
set user stagein
    [Arguments]  ${script}
    set test variable  ${BB_STGIN_USER}  ${script}

set user stageout1
    [Arguments]  ${script}
    set test variable  ${BB_STGOUT1_USER}  ${script}

set user stageout2
    [Arguments]  ${script}
    set test variable  ${BB_STGOUT2_USER}  ${script}

set num computes
    [Arguments]  ${val}
    set test variable  ${numcomputes}  ${val}

set ppn
    [Arguments]  ${val}
    set test variable  ${ppn}  ${val}
    set test variable  ${jsrun}  jsrun -r ${ppn}

bsub&wait
        [Arguments]  ${runcmd}  ${expect_rc}=0  ${timeout}=10mins
	bsub  bsubwaiter  ${runcmd}  ${expect_rc}
	${result}=  waitproc  bsubwaiter  ${expect_rc}  ${timeout}
        [Return]  ${result}

bsub
	[Arguments]  ${handle}  ${runcmd}  ${expect_rc}=0
	set test variable  ${STAGEFULL}  ${empty}
	set test variable  ${STAGE}  ${empty}
	
        run keyword if  ${BB_SSD_MIN} > 0    set test variable  ${STAGE}  ${STAGE}storage=${BB_SSD_MIN}
        run keyword if  ${BB_SSD_MAX} > 0    set test variable  ${STAGE}  ${STAGE},${BB_SSD_MAX}
	
        run keyword if  "${BB_STGIN_USER}" != "${empty}"  set test variable  ${STAGE}  ${STAGE}:in=${BB_STGIN_USER}

	set test variable  ${STGOUT}  ${BB_STGOUT1_USER}
	run keyword if  "${BB_STGOUT2_USER}" != "${empty}"  set test variable  ${STGOUT}  ${BB_STGOUT1_USER},${BB_STGOUT2_USER}
        run keyword if  "${STGOUT}" != "${empty}"  set test variable  ${STAGE}  ${STAGE}:out=${STGOUT}
	
        run keyword if  "${STAGE}" != "${empty}"  set test variable  ${STAGEFULL}  -stage ${STAGE}

	forkproc  ${handle}  bsub -K -a ${application} -o %J.out -e %J.err -q bbq -env all -nnodes ${numcomputes} ${STAGEFULL} ${runcmd}
