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


Perform start a transfer to SSD and perform admin failure during transfer
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
