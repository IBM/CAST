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
     :FOR  ${i}   in range   1   10
     \  bsub  job${i}  hostname
     
     :FOR  ${i}   in range   1   10
     \  waitproc  job${i}  0

Load job queue with BSCFS
     [Tags]  lsf
     using SSD  100
     using bscfs

     :FOR  ${i}   in range   1   10
     \  bsub  job${i}  hostname
     
     :FOR  ${i}   in range   1   10
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
	bsub&wait  hostname  255

Stage-out2 script does not exist
	[Tags]  lsf
	[Timeout]  1 minute
	Using SSD  10
	Set user stageout2  this-script-does-not-exist
	bsub&wait  hostname  255

Stage-in script is started
	 [Tags]  lsf
	 [Timeout]  1 minute
	 Using SSD  10
	 set environment variable  STAGEIN_DONE  ${PFSDIR}/done
	 Set user stagein  ${WORKDIR}/bb/tests/bin/stagein.pl
	 bsub&wait  hostname
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
