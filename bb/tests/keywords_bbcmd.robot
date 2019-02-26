*** Settings ***
Library           ${WORKDIR}/bb/tests/bbRobotLibrary.py  ${WORKDIR}  ${CONFIG}
Library           String


*** Keywords ***
Create directory
    [Arguments]    ${path}  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd    mkdir    --path  ${path}
    Status should be  ${expect_rc}
    Append to list  ${MOUNTDIRS}  ${path}

Change Owner and Group
    [Arguments]    ${path}  ${user}  ${group}  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd    chown    --path=${path}  --user=${user}  --group=${group}
    Status should be  ${expect_rc}

Change Mode
    [Arguments]  ${path}  ${mode}  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd  chmod  --path=${path}  --mode=${mode}
    Status should be  ${expect_rc}

Create Logical Volume
    [Arguments]  ${path}  ${size}  ${expect_rc}=0
    [Timeout]  60 seconds
    bbcmd  create  --mount=${path}  --size=${size}
    Status should be  ${expect_rc}
    Append to list  ${MOUNTLVS}  ${path}

Resize Logical Volume
    [Arguments]  ${path}  ${size}  ${roptions}=BB_NONE  ${expect_rc}=0
    [Timeout]  60 seconds
    Directory should exist  ${path}  "${path} does not exist!"
    Should be true  '${roptions}' == 'BB_NONE' or '${roptions}' == 'BB_DO_NOT_PRESERVE_FS'
    bbcmd  resize  --mount=${path}  --roptions=${roptions}   --size=${size}
    Status should be  ${expect_rc}
    Append to list  ${MOUNTLVS}  ${path}

Resize Logical Volume No ROPTIONCHECK
    [Arguments]  ${path}  ${size}  ${roptions}=BB_NONE  ${expect_rc}=0
    [Timeout]  60 seconds
    Directory should exist  ${path}  "${path} does not exist!"
    bbcmd  resize  --mount=${path}  --roptions=${roptions}   --size=${size}
    Status should be  ${expect_rc}
    Append to list  ${MOUNTLVS}  ${path}

Resize Logical Volume check ROPTION default
    [Arguments]  ${path}  ${size}  ${expect_rc}=0
    [Timeout]  60 seconds
    Directory should exist  ${path}  "${path} does not exist!"
    bbcmd  resize  --mount=${path}  --size=${size}
    Status should be  ${expect_rc}
    Append to list  ${MOUNTLVS}  ${path}

Set Logical Volume Throttle Rate
    [Arguments]  ${path}  ${rate}  ${expect_rc}=0
    [Timeout]  60 seconds
    bbcmd  setthrottle  --mount=${path}  --rate=${rate}
    Status should be  ${expect_rc}

Get transfer handle
    [Arguments]  ${tag}  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd  gethandle  --tag=${tag}  --contrib=${RANKLIST}
    Status should be  ${expect_rc}
    ${handle}=  read transfer handle  0
    [return]  ${handle}

Start file transfer
    [Arguments]  ${handle}  ${filelist}  ${expect_rc}=0
    [Timeout]  60 seconds
    bbcmd  copy  --handle=${handle}  --filelist=${filelist}
    Status should be  ${expect_rc}
	
Get transfer handle list
    [Arguments]  ${expect_rc}=0
    [Timeout]  10 seconds
    bbcmd  gettransfers
    Status should be  ${expect_rc}
    Return from keyword if  ${expect_rc} != 0
    ${handles}=  read transfer handle list  0
    ${rc}  ${handles} =  Run and Return RC and Output  echo "${handles}" | awk '{ print $1 }' | tr -d '()'
    @{handle_list}=  Split String  ${handles}  ,
    [return]  @{handle_list}

Cancel file transfer
    [Arguments]  ${handle}  ${expect_rc}=0
    [Timeout]  60 seconds
    bbcmd  cancel  --handle=${handle}
    Status should be  ${expect_rc}

open connection
     [Arguments]  ${server}  ${expect_rc}=0
     bbcmd  setserver  --open=${server}
     Status should be  ${expect_rc}

close connection
     [Arguments]  ${server}  ${expect_rc}=0
     bbcmd  setserver  --close=${server}
     Status should be  ${expect_rc}

offline connection
     [Arguments]  ${server}  ${expect_rc}=0
     bbcmd  setserver  --offline=${server}
     Status should be  ${expect_rc}

activate connection
     [Arguments]  ${server}  ${expect_rc}=0
     bbcmd  setserver  --activate=${server}
     Status should be  ${expect_rc}

for all ranks
    [Arguments]  ${keyword}  @{varargs}
    @{allranks} =  split string  ${RANKLIST}  ,
    :FOR  ${rank}  in  @{allranks}
    \	  Run keyword  ${keyword}  ${rank}  @{varargs}

Check write threshold
    [Arguments]  ${th}  ${mp}

    broken  Testcase does not parse RAS events correctly

    ${th} =  Run keyword if   ${th} > 0  Set variable  ${th}  ELSE  Set variable  100
    bbcmd    setusagelimit  --mount=${mp}  --wl=${th}

    Sleep   2s  "Sleeping 2 Seconds"
    set test variable  ${source}    ${PFSDIR}
    set test variable  ${dest}      ${mp}
    set test variable  ${maxfiles}  10
    set test variable  ${minsize}   1024
    Generate File List  ${source}  ${dest}  ${mp}/filelist

    bbcmd  gethandle  --tag=0  --contrib=${RANKLIST}
    Status should be  0
    ${handle}=  read transfer handle  0
    bbcmd  copy  --handle=${handle}  --filelist=${mp}/filelist
    Status should be  0
    Sleep   60s  "Sleeping 60 Seconds"
    ${rc}  ${console_line}=  Run and Return RC and Output  tail -30 /var/log/bbproxy.${TESTUSER}/console.log |grep BBWriteUsageExceeded
    Should not be empty   ${console_line}  "RAS EVENT BBWriteUsageExceeded NOT FOUND: for ${MOUNTPT}"
    bbcmd    setusagelimit  --mount=${mp}  --wl=0

Check read threshold
    [Arguments]  ${th}  ${mp}

    broken  Testcase does not parse RAS events correctly

    ${th} =  Run keyword if   ${th} > 0  Set variable  ${th}  ELSE  Set variable  100
    bbcmd    setusagelimit  --mount=${mp}  --rl=${th}

    Sleep   2s  "Sleeping 2 Seconds"
    set test variable  ${source}    ${PFSDIR}
    set test variable  ${dest}      ${mp}
    set test variable  ${maxfiles}  10
    set test variable  ${minsize}   1024
    Generate File List  ${source}  ${dest}  ${mp}/filelist

    bbcmd  gethandle  --tag=0  --contrib=${RANKLIST}
    Status should be  0
    ${handle}=  read transfer handle  0
    bbcmd  copy  --handle=${handle}  --filelist=${mp}/filelist
    Status should be  0
    Sleep   60s  "Sleeping 60 Seconds"
    ${rc}  ${console_line}=  Run and Return RC and Output  tail -30 /var/log/bbproxy.${TESTUSER}/console.log |grep BBReadUsageExceeded
    Should not be empty   ${console_line}  "RAS EVENT BBReadUsageExceeded NOT FOUND: for ${MOUNTPT}"
    bbcmd    setusagelimit  --mount=${mp}  --rl=0



