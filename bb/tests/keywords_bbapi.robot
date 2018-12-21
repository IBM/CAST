*** Settings ***
Library           ${WORKDIR}/bb/tests/bbRobotLibrary.py  ${WORKDIR}  ${CONFIG}
Library           String


*** Keywords ***
API_Create directory
    [Arguments]    ${path}  ${expect_rc}=0
    [Timeout]  10 seconds
    mpirun  ${WORKDIR}/bb/tests/bin/creatDir ${path}
    Append to list  ${MOUNTDIRS}  ${path}

API_Change Owner and Group
    [Arguments]    ${path}  ${user}  ${group}  ${expect_rc}=0
    [Timeout]  10 seconds
    mpirun  ${WORKDIR}/bb/tests/bin/changeOwner ${user} ${group} ${path}

API_Change Mode
    [Arguments]  ${path}  ${mode}  ${expect_rc}=0
    [Timeout]  10 seconds
    mpirun  ${WORKDIR}/bb/tests/bin/changeMode ${path} ${mode}

API_Create Logical Volume
    [Arguments]  ${parms}  ${expect_rc}=0
    [Timeout]  60 seconds
    broken  testcase broken, regular user cannot do this.
    mpirun  ${WORKDIR}/bb/tests/bin/bb_lv ${parms}  ${expect_rc}

API_Resize Logical Volume
    [Arguments]  ${path}  ${size}  ${roptions}=BB_NONE  ${expect_rc}=0
    [Timeout]  60 seconds
    broken  testcase broken, regular user cannot do this.
    Directory should exist  ${path}  "${path} does not exist!"
    Should be true  '${roptions}' == 'BB_NONE' or '${roptions}' == 'BB_DO_NOT_PRESERVE_FS'
    ${result} =  Run keyword if   '${roptions}' == 'BB_DO_NOT_PRESERVE_FS'  mpirun  ${WORKDIR}/bb/tests/bin/resizeLV --mount=${path} --size=${size} -p  ELSE  mpirun  ${WORKDIR}/bb/tests/bin/resizeLV --mount=${path} --size=${size}
    Append to list  ${MOUNTLVS}  ${path}


API_Resize Logical Volume check ROPTION default
    [Arguments]  ${path}  ${size}  ${expect_rc}=0
    [Timeout]  60 seconds
    broken  testcase broken, regular user cannot do this.
    Directory should exist  ${path}  "${path} does not exist!"
    ${result} =  mpirun  ${WORKDIR}/bb/tests/bin/resizeLV --mount=${path} --size=${size} 
    Append to list  ${MOUNTLVS}  ${path}

API_Set Logical Volume Throttle Rate
    [Arguments]  ${path}  ${rate}  ${expect_rc}=0
    [Timeout]  60 seconds
    mpirun  ${WORKDIR}/bb/tests/bin/transfer_rate --mount=${path} --setThrottle=${size} 

API_Get transfer handle
    [Arguments]  ${tag}  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd  gethandle  --tag=${tag}  --contrib=${RANKLIST}
    Status should be  ${expect_rc}
    ${handle}=  read transfer handle  0
    [return]  ${handle}

API_Start file transfer
    [Arguments]  ${filelist}  ${expect_rc}=0
    [Timeout]  60 seconds
    mpirun  ${WORKDIR}/bb/tests/bin/transfer_list ${filelist}
	
API_Get transfer handle list
    [Arguments]  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd  gettransfers
    Status should be  ${expect_rc}
    ${handles}=  read transfer handle list  0
    ${rc}  ${handles} =  Run and Return RC and Output  echo "${handles}" | awk '{ print $1 }' | tr -d '()'
    @{handle_list}=  Split String  ${handles}  ,
    [return]  @{handle_list}

API_Check write threshold
    [Arguments]  ${th}  ${mp}  ${expect_rc}=0

    broken  Testcase does not parse RAS events correctly
    ${th} =  Run keyword if   ${th} > 0  Set variable  ${th}  ELSE  Set variable  100
    mpirun  ${WORKDIR}/bb/tests/bin/setUsageLimit --mount=${mp} -u ${th}

    Sleep   2s  "Sleeping 2 Seconds"
    set test variable  ${source}    ${PFSDIR}
    set test variable  ${dest}      ${mp}
    set test variable  ${maxfiles}  10
    set test variable  ${minsize}   1024

    Generate File List  ${source}  ${dest}  ${mp}/filelist
    API_Run a file transfer  ${mp}/filelist
    Sleep   60s  "Sleeping 60 Seconds"

    ${rc}  ${console_line}=  Run and Return RC and Output  tail -30 /var/log/bbproxy.${TESTUSER}/console.log |grep BBWriteUsageExceeded
    Should not be empty   ${console_line}  "RAS EVENT BBWriteUsageExceeded NOT FOUND: for ${mp}"
    mpirun  ${WORKDIR}/bb/tests/bin/setUsageLimit --mount=${mp} -u 0 

API_Check read threshold
    [Arguments]  ${th}  ${mp}  ${expect_rc}=0
    
    broken  Testcase does not parse RAS events correctly
    ${th} =  Run keyword if   ${th} > 0  Set variable  ${th}  ELSE  Set variable  100
    mpirun  ${WORKDIR}/bb/tests/bin/setUsageLimit --mount=${mp} -t ${th}

    Sleep   2s  "Sleeping 2 Seconds"
    set test variable  ${source}    ${PFSDIR}
    set test variable  ${dest}      ${mp}
    set test variable  ${maxfiles}  10
    set test variable  ${minsize}   1024
    Generate File List  ${source}  ${dest}  ${mp}/filelist
	
    API_Run a file transfer  ${mp}/filelist
    Sleep   60s  "Sleeping 60 Seconds"

    ${rc}  ${console_line}=  Run and Return RC and Output  tail -30 /var/log/bbproxy.${TESTUSER}/console.log |grep BBReadUsageExceeded
    Should not be empty   ${console_line}  "RAS EVENT BBReadUsageExceeded NOT FOUND: for ${mp}"
    mpirun  ${WORKDIR}/bb/tests/bin/setUsageLimit --mount=${mp} -t 0 

API_Get last error
    [Arguments]   ${expect_rc}=0
    [Timeout]  10 seconds
    ${result} =  mpirun  ${WORKDIR}/bb/tests/bin/getLastErr
    ${result} =  mpirun  ${WORKDIR}/bb/tests/bin/getLastErr NULL


API_Validate User can initialize BB Environment
    [Arguments]   ${user}   ${expect_rc}=0
    broken  Testcase requires root to add files
    
    ${rc}=  Run and Return RC  /sbin/useradd ${user} 
    Should be equal as integers  ${rc}  0
    ${rc}=  Run and Return RC  /usr/bin/su - ${user} -c "${WORKDIR}/bb/tests/bin/bb_init --dumpJSON=/tmp/JSON.out"
    ${rc}=  Run keyword unless  "${rc}" != "0"  Run and Return RC  /sbin/userdel -r ${user} 
    File should exist  /tmp/JSON.out
    File should exist  /usr/bin/jq 
    ${rc}  ${result}=  Run and Return RC and Output  /usr/bin/cat /tmp/JSON.out | /usr/bin/jq .'rc' | tr -d '"'
    Remove file  /tmp/JSON.out
    Should be equal as integers  ${rc}  0
    Should be equal as integers  ${result}  ${expect_rc}


