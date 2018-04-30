*** Settings ***
Resource 	  ../common.robot
Suite Setup       Setup test environment
Test Setup        Setup testcases
Test Teardown     Cleanup transfer test

*** Keywords ***
Setup Testcases
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

*** Test Cases ***

Basic py transfers
    [Tags]  py  transfer
    runpytest  bbtests  --mount=${MOUNTPT} --orgsrc=${PFSDIR} --jobid=${JOBID} --jobstepid=${JOBSTEPID}
