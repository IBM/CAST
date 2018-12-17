*** Settings ***
Resource          ../common.robot
Resource	  tools.robot
Suite Setup       setup resiliency environment
Suite teardown    teardown resiliency environment
Test Teardown     Failover back to primary

*** Test Cases ***
Basic admin failover
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    
    start job
    Run as root
    Use hosts with defined server  backup
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

Perform transfer to SSD followed by admin failover 
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Setup]  Setup to SSD
    [Timeout]  10 minutes
        
    Run parameterized transfer
    
    Run as root
    Use hosts with defined server  backup
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

Perform transfer from SSD followed by admin failover 
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup from SSD

    Run parameterized transfer
    
    Run as root
    Use hosts with defined server  backup
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

Perform admin failover followed by transfer to SSD
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup to SSD
    Run as root
    Use hosts with defined server  backup
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Run as user
    Run parameterized transfer
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

Perform admin failover followed by transfer from SSD
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup from SSD
    Run as root
    Use hosts with defined server  backup
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Run as user
    Run parameterized transfer
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}


Perform admin failover during transfer->SSD
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup to SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}


Perform 2 admin failovers during transfer->SSD
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup to SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist
    

Perform admin failover during transfer->SSD and another failover during 2nd transfer->SSD
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup to SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Run keyword and ignore error  Create directory  ${source}/tmp
    Create directory  ${dest}/tmp
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    Generate File List  ${source}/tmp  ${dest}/tmp  ${MOUNTPT}/filelist2

    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}

    ${handle2}=  Run a file transfer  2  ${MOUNTPT}/filelist2
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist

    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle2}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist2
    Remove Random files   ${MOUNTPT}/filelist2

    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}


Perform admin failover during transfer->PFS
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup from SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist
    
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}


Perform 2 admin failovers during transfer->PFS
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup from SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}
    
    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist
    

Perform admin failover during transfer->PFS and another failover during 2nd transfer->PFS
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    [Setup]  Setup from SSD
    
    set test variable  ${minsize}   8589934592
    set test variable  ${maxsize}   8589934592
    
    Run as root
    Use hosts with defined server  backup
    
    Run as user
    Run keyword and ignore error  Create directory  ${source}/tmp
    Run keyword and ignore error  Create directory  ${dest}/tmp
    Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
    Generate File List  ${source}/tmp  ${dest}/tmp  ${MOUNTPT}/filelist2

    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=backup --hosts=${HOSTLIST} --jobid=${JOBID}

    ${handle2}=  Run a file transfer  2  ${MOUNTPT}/filelist2
    sleep  1
    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}

    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist
    Remove Random files   ${MOUNTPT}/filelist

    Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle2}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Compare Random files  ${MOUNTPT}/filelist2
    Remove Random files   ${MOUNTPT}/filelist2

    shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}
