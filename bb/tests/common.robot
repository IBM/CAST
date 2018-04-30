*** Settings ***
Resource          parms.robot
Resource          keywords_bbapi.robot
Resource          keywords_bbcmd.robot
Resource          keywords_toolbox.robot
Resource          keywords_lsf.robot
Resource          keywords_pytest.robot
Library           OperatingSystem
Library           Collections


*** Keywords ***
Run as root
     set test variable  ${IAMROOT}  1

Run as user
     set test variable  ${IAMROOT}  0

Broken
     [Arguments]  ${msg}="for reasons"
     Run keyword if  ${SKIPBROKEN} == 1  FAIL  Testcase was marked as broken: ${msg}

Setup CMDLAUNCHER for MPIRUN
     remove environment variable  LSB_JOBID
     set test variable  ${CMDLAUNCHER}   /opt/ibm/spectrum_mpi/bin/mpirun -H ${HOSTLIST} --tag-output -x LSB_JOBID\=${JOBID} -x PMIX_NAMESPACE\=${JOBSTEPID}

Setup CMDLAUNCHER for empty
     set test variable  ${CMDLAUNCHER}   ${EMPTY}

Setup CMDLAUNCHER
     Run keyword unless  '${HOSTLIST}' == ''  Setup CMDLAUNCHER for MPIRUN
     Run keyword if      '${HOSTLIST}' == ''  Setup CMDLAUNCHER for empty
     ${USEMPI}=  set variable if  '${HOSTLIST}' == ''  ${EMPTY}
     ...                          '1' == '1'           -mpi
     
     set test variable  ${RANDFILE}      ${WORKDIR}/bb/tools/randfile${USEMPI}
     set test variable  ${COMPAREFILES}  ${WORKDIR}/bb/tools/comparefiles${USEMPI}
     set test variable  ${REMOVEFILES}   ${WORKDIR}/bb/tools/removefiles${USEMPI}

Restore rank list
    [Arguments]
    set test variable  ${HOSTLIST}  ${ORIG_HOSTLIST}
    set test variable  ${RANKLIST}  ${ORIG_RANKLIST}
    Setup CMDLAUNCHER

Start job
     ${tmpMAXJOBID}=  get file  ${PFSDIR}/maxjob     
     ${MAXJOBID}=  evaluate  ${tmpMAXJOBID} + 1
     ${tmpMAXJOBID}=  Convert to String  ${MAXJOBID}
     create file  ${PFSDIR}/maxjob  ${tmpMAXJOBID}
     
     set test variable  ${JOBID}  ${MAXJOBID}
     set test variable  ${JOBSTEPID}  1
     set environment variable  LSB_JOBID       ${JOBID}
     set environment variable  PMIX_NAMESPACE  ${JOBSTEPID}
     Setup CMDLAUNCHER
     
     [return]  ${JOBID}

Start job step
     ${tmpJOBSTEPID}=  evaluate  ${JOBSTEPID} + 1
     set test variable  ${JOBSTEPID}  ${tmpJOBSTEPID}
     set environment variable  PMIX_NAMESPACE  ${JOBSTEPID}
     Run keyword unless  '${HOSTLIST}' == ''  Setup CMDLAUNCHER for MPIRUN
     Run keyword if      '${HOSTLIST}' == ''  Setup CMDLAUNCHER for empty

Setup test environment
    Log  configuring test environment
    set environment variable  JOBID      1
    set environment variable  JOBSTEPID  0
    Set Suite Variable  ${SUITEISROOT}  1
    Set Suite Variable  ${ORIG_HOSTLIST}  ${HOSTLIST}
    Set Suite Variable  ${ORIG_RANKLIST}  ${RANKLIST}
    Run keyword and continue on failure  bbcmd  remove  --mount=${MOUNTPT}
    Run keyword and continue on failure  bbcmd  rmdir  --path=${MOUNTPT}
    Set Suite Variable  ${SUITEISROOT}  0
    OperatingSystem.Create Directory  ${PFSDIR}

Teardown test environment
    Log  tearing down test environment

Setup api test environment
    Log to console  Starting...

Teardown api test environment
    Log to console  Ending...
