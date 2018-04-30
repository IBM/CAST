*** Settings ***
Resource        ../common.robot
Test Setup      Setup LSF testcases
Suite setup     Clear LSF jobs
Suite teardown  Clear LSF jobs

*** Test Cases ***

Transfer initiated during stage-in
	 [Tags]  lsf
	 [Documentation]  robot creates files.  stagein.pl starts file transfer.  Files are compared during job execution with comparefiles
	 Using SSD  10
         set environment variable  STAGEIN_FILELIST  ${PFSDIR}/lsfin
	 set user stagein  ${WORKDIR}/bb/tests/bin/stagein.pl
	 
	 Generate File List  ${PFSDIR}  \$BBPATH  %{STAGEIN_FILELIST}
	 bsub&wait  ${jsrun} ${WORKDIR}/bb/tools/comparefiles --filelist %{STAGEIN_FILELIST}

Transfer initiated during job execution
	 [Tags]  lsf
	 [Documentation]  launch1.pl creates files, generates md5sums, and starts file transfer.  md5sum is checked via robot.
	 Using SSD  10
	 set environment variable  STAGEOUT_DIR       ${PFSDIR}
         set environment variable  STAGEOUT_FILELIST  ${PFSDIR}/lsfout
	 set environment variable  STAGEOUT_MD5SUM    ${PFSDIR}/lsfout.md5sum
	 
	 bsub&wait  ${WORKDIR}/bb/tests/bin/launch1.pl
	 shellrun with cwd  md5sum -c %{STAGEOUT_MD5SUM}  ${PFSDIR}

Transfer initiated during stage-out1
	 [Tags]  lsf
	 [Documentation]  launch2.pl creates files and generates md5sums, stageout_intiate starts file transfer.  md5sum is checked via robot.
	 Using SSD  10
	 set environment variable  STAGEOUT_DIR       ${PFSDIR}
         set environment variable  STAGEOUT_FILELIST  ${PFSDIR}/lsfout
	 set environment variable  STAGEOUT_MD5SUM    ${PFSDIR}/lsfout.md5sum
	 set user stageout1  ${WORKDIR}/bb/tests/bin/stageout_initiate.pl	 
	 
	 bsub&wait  ${WORKDIR}/bb/tests/bin/launch2.pl
	 shellrun with cwd  md5sum -c %{STAGEOUT_MD5SUM}  ${PFSDIR}

Transfer initiated during stage-in to multiple nodes
	 [Tags]  lsf
	 [Documentation]  robot creates files.  stagein.pl starts file transfer to multiple nodes.  Files are compared during job execution with comparefiles
	 Using SSD  10
	 set num computes  2
         set environment variable  STAGEIN_FILELIST  ${PFSDIR}/lsfin
	 set user stagein  ${WORKDIR}/bb/tests/bin/stagein.pl
	 
	 Generate File List  ${PFSDIR}  \$BBPATH  %{STAGEIN_FILELIST}
	 bsub&wait  ${jsrun} ${WORKDIR}/bb/tools/comparefiles --filelist ${PFSDIR}/lsfin

Transfer initiated during stage-in to multiple processes
	 [Tags]  lsf
	 [Documentation]  robot creates files.  stagein.pl starts file transfer to multiple processes.  Files are compared during job execution with comparefiles
	 Using SSD  10
	 set ppn  2
         set environment variable  STAGEIN_FILELIST  ${PFSDIR}/lsfin
	 set user stagein  ${WORKDIR}/bb/tests/bin/stagein.pl
	 
	 Generate File List  ${PFSDIR}  \$BBPATH  %{STAGEIN_FILELIST}
	 bsub&wait  ${jsrun} ${WORKDIR}/bb/tools/comparefiles --filelist ${PFSDIR}/lsfin


Transfer initiated during stage-out to multiple nodes
	 [Tags]  lsf  todo

Transfer initiated during stage-out to multiple processes
	 [Tags]  lsf  todo
	 
