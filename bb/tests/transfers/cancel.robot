*** Settings ***
Resource 	  ../common.robot
Suite Setup       Setup test environment
Test Setup        Setup testcases
Test Teardown     Cleanup transfer test

*** Keywords ***
Setup Testcases
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  128G
     Run as user

*** Test Cases ***

User can cancel file transfers to SSD
    [Tags]  unittest  transfer  tossd  cancel
     set test variable  ${minsize}   65536
     set test variable  ${maxsize}   65536
     set test variable  ${minfiles}  128
     set test variable  ${maxfiles}  128
     Generate File List  ${PFSDIR}  ${MOUNTPT}  ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
     Cancel file transfer  ${handle}
     Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
     
     ${status} =  read transfer status  0
     Pass Execution if  '${status}' == 'BBFULLSUCCESS'  Warning: Transfer completed before cancel
     
     Transfer status is  ${handle}  BBCANCELED


User can cancel file transfers from SSD
    [Tags]  unittest  transfer  fromssd  cancel
     set test variable  ${minsize}   65536
     set test variable  ${maxsize}   65536
     set test variable  ${minfiles}  128
     set test variable  ${maxfiles}  128
     Generate File List  ${MOUNTPT}  ${PFSDIR}  ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
     Cancel file transfer  ${handle}
     Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}

     ${status} =  read transfer status  0
     Pass Execution if  '${status}' == 'BBFULLSUCCESS'  Warning: Transfer completed before cancel
     
     Transfer status is  ${handle}  BBCANCELED

User can cancel big bunch transfers via API
    [Tags]  fvt  transfer  cancel
    [Timeout]  600 seconds
    set test variable  ${minsize}   65536
    set test variable  ${maxsize}   65536
    set test variable  ${minfiles}  1000
    set test variable  ${maxfiles}  1000
    Generate File List  ${MOUNTPT}  ${PFSDIR}  ${PFSDIR}/filelist
    ${rc}=  Run and Return RC  ${WORKDIR}/bb/tests/bin/xfer_cancel -f ${PFSDIR}/filelist
    Should be equal as integers  ${rc}  0

*** Keywords ***
Run cancel testcase
    [Arguments]  ${numthreads}  ${numfiles}
     mpirun  ${WORKDIR}/bb/tests/bin/cancel_transfer -l ${MOUNTPT} -p ${PFSDIR} -t ${numthreads} -f ${numfiles}
