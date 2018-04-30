*** Settings ***
Resource          ../common.robot
Resource	  tools.robot
Suite Setup       setup resiliency environment
Suite teardown    teardown resiliency environment

*** Test Cases ***
Open and close connection to backup server
    [Documentation]  Perform a simple connect/disconnect on all bbProxys with defined backup servers
    [Tags]  connect
    [Timeout]  10 minutes

    start job
    
    Run as root
    
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup
    
    # Connect/Disconnect
    Open connection  backup
    Close connection  backup
    Restore rank list

Offline primary connection and reactivate
    [Documentation]  Offline the primary connection, verify the bbServer connection is non-functional, reactivate, verify function returned
    [Tags]  connect
    [Timeout]  10 minutes
    
    start job
    Run as root

    # Connect/Disconnect
    Offline connection  primary
    Get transfer handle list  -1
    Activate connection  primary
    Get transfer handle list


Attempt opening connection as user fails
    [Documentation]  Attempt to open a connection as the user.  Connection open should fail.
    [Tags]  connect  badpath
    [Timeout]  10 minutes
    
    start job
    Run as user
    
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup
    
    # Connect/Disconnect
    Open connection  backup  -1
    Restore rank list

Attempt closing connection as user fails
    [Documentation]  Attempt to open a connection as the user.  Connection open should fail.
    [Tags]  connect  badpath
    [Timeout]  10 minutes

    start job
    Run as user

    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    # Connect/Disconnect
    Close connection  backup  -1
    Restore rank list


Open connection twice
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    
    start job
    Run as root
    Open connection  primary
    

Open connection to invalid server should fail
    [Documentation]  Attempt to open a connection to an invalid server.  This should fail.
    [Tags]  connect  badpath
    [Timeout]  10 minutes
    
    start job
    Run as root
    Open connection  heisenbox  107


Close connection to invalid server should fail
    [Documentation]  Close a connection to an invalid server.  This should fail.
    [Tags]  connect  badpath
    [Timeout]  10 minutes
    
    start job
    Run as root
    Close connection  heisenbox  -1


Close connection to unconnected server should pass
    [Documentation]  Attempt to open a connection that is already open.  This should succeed.
    [Tags]  connect
    [Timeout]  10 minutes
    
    start job
    
    Run as root
    
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    Close connection  backup


Open connection to backup server while performing transfer to SSD
    [Documentation]  Perform a connection to backup server while performing transfer to SSD
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${PFSDIR}  ${MOUNTPT}  ${MOUNTPT}/filelist

    # initiate transfer 
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    # Connect to backup server in middle of transfer
    Open connection  backup

    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS

    Cleanup transfer test
    #  Disconnect from backup server
    Close connection  backup
    
    Restore rank list

Execute 100 Times - Open connection to backup server while performing transfer to SSD
    [Documentation]  Perform a connection to backup server while performing transfer to SSD
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${PFSDIR}  ${MOUNTPT}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    :FOR  ${l_idx}   in range   1   100
    \  Open connection  backup
    \  Close connection  backup

    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Cleanup transfer test

    Restore rank list



Open connection to backup server while performing transfer from SSD on primary
    [Documentation]  Perform a simple connect/disconnect on all bbProxys with defined backup servers
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${MOUNTPT}  ${PFSDIR}  ${MOUNTPT}/filelist

    # initiate transfer 
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    # Connect to backup server in middle of transfer
    Open connection  backup

    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS

    Cleanup transfer test
    #  Disconnect from backup server
    Close connection  backup
    
    Restore rank list


Execute 100 Times - Open connection to backup server while performing transfer from SSD on primary
    [Documentation]  Perform a connection to backup server while performing transfer from SSD
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${MOUNTPT}  ${PFSDIR}  ${MOUNTPT}/filelist
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    :FOR  ${l_idx}   in range   1   100
    \  Open connection  backup
    \  Close connection  backup
    
    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS
    Cleanup transfer test

    Restore rank list


Close connection to backup server while performing transfer to SSD on primary
    [Documentation]  Perform a simple connect/disconnect on all bbProxys with defined backup servers
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${PFSDIR}  ${MOUNTPT}  ${MOUNTPT}/filelist

    # initiate transfer 
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    # Connect and close connection to backup server should not impact transfer 
    Open connection  backup
    Close connection  backup

    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS

    Cleanup transfer test
    
    Restore rank list



Close connection to backup server while performing transfer from SSD on primary
    [Documentation]  Perform a simple connect/disconnect on all bbProxys with defined backup servers
    [Tags]  connect
    [Timeout]  10 minutes

    start job

    Run as root
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup

    set test variable  ${minsize}   1073741824
    set test variable  ${maxsize}   1073741824
    Setup logical volume  ${MOUNTPT}  8G
    Generate File List  ${MOUNTPT}  ${PFSDIR}  ${MOUNTPT}/filelist

    # initiate transfer 
    ${handle}=  Run a file transfer  1  ${MOUNTPT}/filelist

    # Connect and close connection to backup server should not impact transfer 
    Open connection  backup
    Close connection  backup

    Wait Until Keyword Succeeds  10 mins  1 second  transfer has completed  ${handle}
    Transfer status is  ${handle}  BBFULLSUCCESS

    Cleanup transfer test
    
    Restore rank list



Connection establishment stress
    [Documentation]  Open/close connection to backup server 100 times in succession.  
    [Tags]  connect
    [Timeout]  10 minutes
    
    start job
    Run as root
    
    # Not all ranks have a backup server defined.  Remove from ranklist for this test.
    Use hosts with defined server  backup
    
    # Connect/Disconnect
    :FOR  ${iteration}  in range  100
    \    Open connection  backup
    \	 Close connection  backup
    
    Restore rank list
