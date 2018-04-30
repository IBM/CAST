*** Settings ***
Library           OperatingSystem
Library           Collections
Library           Process
Library           String
Resource          ../common.robot


*** Test Cases ***

API Verify BB_GetVersion
    [Tags]  fvt  bbapi
    [Documentation]  
    ...		     test_ver.c testcase
    ...		     Calls BB_GetVersion(), test for correct and invalid buffer size.   
    Start job
    Run as root
    API Test BB_GetVersion
    API Test BB_GetVersion    "80"      255

API get version too small
    [Tags]  fvt  bbapi
    [Documentation]  
    ...		     get_version2.c testcase
    ...		     Call BB_GetVersion() with too little buffer space for the string.  Confirm failure.
    
    Start job
    Run as user
    mpirun  ${WORKDIR}/bb/tests/bin/get_version2


API_User cannot initialize BB_API Environment with NULL Value
    [Tags]  fvt
    [Documentation]  FVT Testing BB_InitLibraryPI_User can initialize BB_API Environment
    ...           API initialization   Should succeed on RC=255 
    Start job
    Run as root
    API initialization  -s ""   255
	
API_User can initialize BB_API Environment
    [Tags]  fvt
    [Documentation]  FVT Testing BB_InitLibrary
    ...           API initialization   Should succeed  
    Start job
    Run as root
    API initialization

API_User cannot initialize BB_API Env with Bogus version String
    Start job
    Run as root
    API initialization  -s "{^version^:{^major^:^0^,^minor^:^0^},^gitcommit^:^ed4d5BOGUS_VALUEc9756ed3a9c68c1^}"   255

	

API initialization with bbProxy down
    [Tags]  todo  fvt
    [Documentation]  
    ...		     init3.c
    ...		     Pass in a bogus configuration file that points to a non-existant bbProxy
    ...		     Failures should be produced.  
    ...		     Current init3.c assumes setup failure in bbProxy.  More work is needed here for robot environment.
    ...		     Run as manual testcase?  Or inject config error?  tbd
    broken  test not implemented

API User cannot Initialize BB Environment TWICE
    [Tags]  fvt
    [Documentation]  
    ...		     bb_init --invokeTwice  
    ...		     Expected to SUCCEED.  
    ...		     Testcase invokes BB_InitLibrary twice.   Second time generates error.
    Start job
    Run as root
    API initialization  --invokeTwice
	
API User cannot Initialize BB Environment TWICE with Dif Credntls 
    [Tags]  fvt
    [Documentation]  
    ...		     bb_init --invokeTwiceDifCredtls  
    ...		     Expected to SUCCEED.  
    ...		     Testcase invokes BB_InitLibrary twice.   Second time generates error.
    Start job
    Run as root
    API initialization  --invokeTwiceDifCredtls
	
API init during C runtime initialization
    [Tags]  bbapi
    [Documentation]
    ...  Spectral suite intercepts the glibc I/O library and potentially could call bbAPI
    ...  before the C++ runtime has initialized.  This test mimics Spectral and calls BB_InitLibrary
    ...  during this early init window.  BB_InitLibrary() and BB_Terminatelibray() should fail with a 
    ...  bad return code.
    
    Start job
    ${result} =   mpirun  ${WORKDIR}/bb/tests/bin/earlyInit


API init from many threads
    [Tags]  bbapi
    [Documentation]
    ...   Initializes bbAPI from many threads simulaenously.  Only one thread should be successful.
    
    Start job
    ${result} =   mpirun  ${WORKDIR}/bb/tests/bin/initThreadSafety


API term from many threads
    [Tags]  bbapi
    [Documentation]
    ...   Terminates bbAPI from many threads simulaenously.  Only one thread should be successful.
    
    Start job
    ${result} =   mpirun  ${WORKDIR}/bb/tests/bin/termThreadSafety

API init/term loop
    [Tags]  bbapi
    [Documentation]
    ...  Testcase to loop BB_InitLibrary and BB_TerminateLibrary.  Testcase should be successful.
    Start job
    ${result} =   mpirun  ${WORKDIR}/bb/tests/bin/initLoopSafety


*** Keywords ***

API initialization
    [Arguments]  ${parms}=""   ${expect_rc}=0
    ${result} =  Run keyword if  '${parms}' == ''  run  ${WORKDIR}/bb/tests/bin/bb_init  ${expect_rc}  ELSE  mpirun  ${WORKDIR}/bb/tests/bin/bb_init ${parms}   ${expect_rc}


API Test BB_GetVersion
    [Arguments]   ${size}="256"    ${expect_rc}=0
    ${result} =  mpirun  ${WORKDIR}/bb/tests/bin/test_ver --szBuffer=${size}    ${expect_rc}
