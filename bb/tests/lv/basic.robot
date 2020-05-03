*** Settings ***
Resource          ../common.robot
Suite Setup       setup test environment


*** Test Cases ***
User can create directory
    [Documentation]  Creates directory at mount point
    [Tags]  unittest  directory
    Create directory    ${MOUNTPT}
    Status Should Be   0
    [TEARDOWN]  Remove directories

User cannot create directory to bad path
     [Documentation]  Attempts to see if bbAPI received a directory error shortly after the crash.
     [Tags]  unittest  directory  badpath
     Create directory   /non-existant/foobar  2

User cannot create directory twice
    [Tags]  unittest  directory  badpath
     Create directory   ${MOUNTPT}
     Status should be  0
     Create directory   ${MOUNTPT}  17
     [TEARDOWN]  Remove directories

User can create nested directories
    [Tags]  unittest  directory
     Create directory  ${MOUNTPT}
     Status should be  0
     Create directory  ${MOUNTPT}/qux
     Status should be  0
     [TEARDOWN]  Remove directories

User can change directory ownership
    [Tags]  unittest  directory
     Create directory   ${MOUNTPT}
     Status should be  0
     ${prigroup}=  Get Primary Group  ${TESTUSER}
     Change owner and group  ${MOUNTPT}  ${TESTUSER}  ${prigroup}
     Status should be  0
     [TEARDOWN]  Remove directories

User can change directory permissions
    [Tags]  unittest  directory
     Create directory  ${MOUNTPT}
     Status should be  0
     Change mode  ${MOUNTPT}  0777
     Status should be  0
     [Teardown]  Remove directories

User can create logical volume
    [Tags]  unittest  lv
    Start job
    Run as root
    FOR  ${size}  IN  132M  1G  2G  100G  800G
       Setup logical volume  ${MOUNTPT}  ${size}
       Verify block  ${MOUNTPT}  ${size}  4M
       Teardown logical volume  ${MOUNTPT}
    END
    [TEARDOWN]  Teardown logical volume  ${MOUNTPT}

User can increase size of logical volume
    [Tags]  unittest  lv
    [Template]  Change LV Size
    2G	     4G        4G	BB_NONE
    20G	     25.25G    25.25G	BB_NONE
    25.25G   +4G       29.25G	BB_NONE
    100G     800G      800G	BB_NONE
    2G	     4G        4G	BB_DO_NOT_PRESERVE_FS
    20G	     25.25G    25.25G	BB_DO_NOT_PRESERVE_FS
    25.25G   +4G       29.25G	BB_DO_NOT_PRESERVE_FS
    100G     800G      800G	BB_DO_NOT_PRESERVE_FS

User can increase size of logical volume BAD_ROPTION
    [Tags]  unittest  lv
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  1G  
     Resize Logical Volume No ROPTIONCHECK  ${MOUNTPT}  2G  BAD_ROPTION  -1

     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Teardown logical volume  ${MOUNTPT}

User can increase size of logical no roption specified
    [Tags]  unittest  lv
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  1G  
     Resize Logical Volume check ROPTION default  ${MOUNTPT}  2G

     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Teardown logical volume  ${MOUNTPT}

User cannot create logical volume with invalid size
    [Tags]  unittest  lv  badpath
    Start job
    Run as root
    FOR  ${size}  IN  -1G  0  1  1M  4M  7000G
        Setup logical volume  ${MOUNTPT}  ${size}  -1
        Teardown logical volume  ${MOUNTPT}
    END
    [TEARDOWN]  Teardown logical volume  ${MOUNTPT}

API_User can create directory
    [Documentation]  Creates directory at mount point
    [Tags]  unittest  directory
    Start job
    API_Create directory   ${MOUNTPT}
    [TEARDOWN]  Remove directories

API_Validate Format of Random BB JSON
    [Documentation]  Gets random JSON from BB_GetLastErrorDetails and test format 
    [Tags]  unittest  directory
    Start job
    API_Get last error
    [TEARDOWN]  Remove directories


API User can Initialize BB Environment with Server running on different Node
    [Tags]  todo
    Start job
    Run as root
    API_Validate User can initialize BB Environment   fvt-bb-test-usr1


*** Keywords ***
Change LV Size
     [Arguments]  ${initialsize}  ${modsize}  ${expectsize}  ${flags}=${EMPTY}

     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  ${initialsize}
     Resize logical volume  ${MOUNTPT}  ${modsize}  ${flags}
     Verify block  ${MOUNTPT}  ${expectsize}  4M

     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Teardown logical volume  ${MOUNTPT}
