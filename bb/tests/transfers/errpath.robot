*** Settings ***
Resource 	  ../common.robot
Suite Setup       Setup test environment
Test Setup        Setup testcases
Test Teardown     Cleanup transfer test

*** Keywords ***
Setup Testcases
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  4G
     Run as user

*** Test Cases ***
Bad parameters for transfer APIs
     [Tags]  unittest  transfer  tossd  badpath
     mpirun  ${WORKDIR}/bb/tests/bin/parmchecktransfer

Bad parameters for get version API
     [Tags]  unittest  badpath
     mpirun  ${WORKDIR}/bb/tests/bin/parmcheckGetVersion

Null handle for transfer
    [Tags]  unittest  transfer  tossd  badpath
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}/fake
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     Remove Random files   ${MOUNTPT}/filelist
     Start file transfer  0  ${MOUNTPT}/filelist  2

NonNull bad handle for transfer
    [Tags]  unittest  transfer  tossd  badpath
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}/fake
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     Remove Random files   ${MOUNTPT}/filelist
     Start file transfer  1  ${MOUNTPT}/filelist  2
    

Transfer files to non-existent SSD path should fail
    [Tags]  unittest  transfer  tossd  badpath
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}/fake
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     Remove Random files   ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist  2


Transfer files to non-existent PFS mount point should fail
     [Tags]  unittest  transfer  fromssd  badpath
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}/fake
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     Remove Random files   ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist  2

Transfer files to tmpfs should fail
    [Tags]  unittest  transfer  tossd  badpath
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      /tmp
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist  -1

Transfer files from tmpfs should fail
     [Tags]  unittest  transfer  fromssd  badpath
     set test variable  ${source}    /tmp
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist  -1


Transfer file to SSD is larger than LV should fail
    [Tags]  unittest  transfer  tossd  badpath
    [Setup]
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  192M
     Run as user
     
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   268435456
     set test variable  ${maxsize}   268435457
     
     Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist  -1

Verify transfer status query of non-contributing task
    [Tags]  fvt  transfer  badpath
    [Setup]
    Start job
    Run as root
    Setup logical volume  ${MOUNTPT}  192M
    Run as user
    
    ok to run mpi executable
    
    @{_tmp}=  Split String  ${HOSTLIST}  ,
    ${hostl}=  create list  @{_tmp}
    ${cnt}=  get length  ${hostl}
    Pass Execution If  ${cnt} < 3  Test requires at least 3 nodes
    
    mpirun  ${WORKDIR}/bb/tests/bin/getTransferInfo ${PFSDIR} ${MOUNTPT}
