*** Settings ***
Resource 	  ../common.robot
Suite Setup       Setup test environment
Test Setup        Setup testcases
Test Teardown     Cleanup transfer test

*** Keywords ***
Setup Testcases
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

*** Test Cases ***

User can transfer files to SSD
    [Tags]  unittest  transfer  tossd
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer

User transfer files to SSD usage check
    [Tags]  unittest  transfer  tossd  usage
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     bbcmd  getusage  --mount=${MOUNTPT}
     ${start_burst_write}=  read device data  burstBytesWritten  0
     Run parameterized transfer
     bbcmd  getusage  --mount=${MOUNTPT}
     ${end_burst_write}=  read device data  burstBytesWritten  0
     ${burst_write}=  Evaluate  ${end_burst_write}-${start_burst_write}
     Should Be Equal As Integers  ${burst_write}  1048576

User transfer 8G file to SSD usage check
    [Tags]  unittest  transfer  tossd  usage
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   8589934592
     set test variable  ${maxsize}   8589934592
     bbcmd  getusage  --mount=${MOUNTPT}
     ${start_burst_write}=  read device data  burstBytesWritten  0
     Run parameterized transfer
     bbcmd  getusage  --mount=${MOUNTPT}
     ${end_burst_write}=  read device data  burstBytesWritten  0
     ${burst_write}=  Evaluate  ${end_burst_write}-${start_burst_write}
     Should Be Equal As Integers  ${burst_write}  8589934592

User can transfer files from SSD
     [Tags]  unittest  transfer  fromssd
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer


User transfer files from SSD usage check
     [Tags]  unittest  transfer  fromssd  usage
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     bbcmd  getusage  --mount=${MOUNTPT}
     ${start_burst_read}=  read device data  burstBytesRead  0
     Run parameterized transfer
     bbcmd  getusage  --mount=${MOUNTPT}
     ${end_burst_read}=  read device data  burstBytesRead  0
     ${burst_read}=  Evaluate  ${end_burst_read}-${start_burst_read}
     Should Be Equal As Integers  ${burst_read}  1048576

User transfer 8G file from SSD usage check
     [Tags]  unittest  transfer  fromssd  usage
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   8589934592
     set test variable  ${maxsize}   8589934592
     bbcmd  getusage  --mount=${MOUNTPT}
     ${start_burst_read}=  read device data  burstBytesRead  0
     Run parameterized transfer
     bbcmd  getusage  --mount=${MOUNTPT}
     ${end_burst_read}=  read device data  burstBytesRead  0
     ${burst_read}=  Evaluate  ${end_burst_read}-${start_burst_read}
     Should Be Equal As Integers  ${burst_read}  8589934592

User can start an empty transfer
     [Tags]  unittest  transfer  fromssd
     set test variable  ${source}        ${MOUNTPT}
     set test variable  ${dest}          ${PFSDIR}
     set test variable  ${numiter}       1
     set test variable  ${minfiles}      0
     set test variable  ${maxfiles}      0
     set test variable  ${polltimeout}   60 secs
     Run parameterized transfer

User can transfer zero byte file from SSD
     [Tags]  unittest  transfer  fromssd
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   0
     set test variable  ${maxsize}   0
     Run parameterized transfer


User can transfer zero byte file to SSD
     [Tags]  unittest  transfer  tossd
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   0
     set test variable  ${maxsize}   0
     Run parameterized transfer

API User can addfiles with VALID BBFILEFLAGS  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --noTransferDef --bbFlag --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source/target file to a non-existent transfer definition
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M
    
    API Test Addfiles  --bbFFlag=0x0001 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
    API Test Addfiles  --bbFFlag=0 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
    API Test Addfiles  --bbFFlag=4 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
    API Test Addfiles  --bbFFlag=8 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
	
    ${rc}=  Run and Return RC  rm -rf ${MOUNTPT}/masterfile
    Should be equal as integers  ${rc}  0	 
	 	 
API User cannot addfiles with Non-Existent Source  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --noTransferDef --bbFlag --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source/target file to a non-existent transfer definition
    
    API Test Addfiles  --bbFFlag=0 --srcFile=FVT-TEST-INVALID-SRCFILE --targetFile=${PFSDIR}   8
 	
API User can get Throttle Rate  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   transfer_rate --getThrottle --mount --dumpJson 
    ...  	     ERROR: Testing BB_GetThrottle Rate 
    
    API Test Transfer_rate   -g -m ${MOUNTPT}
	
API User can set Throttle Rate  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   transfer_rate --setThrottle --newRate --mount --dumpJson 
    ...  	     ERROR: Testing BB_GetThrottle Rate  
    
    API Test Transfer_rate   -s -n 4445555 -m ${MOUNTPT}
	
	

API User cannot addfiles with Invalid BBFILEFLAGS  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --noTransferDef --bbFlag --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source/target file to a non-existent transfer definition
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M

    API Test Addfiles  --bbFFlag=0xffff --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}   8
    API Test Addfiles  --bbFFlag=6 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}        8
    API Test Addfiles  --bbFFlag=0x0030 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}   8  
    API Test Addfiles  --bbFFlag=0x00c0 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}   8
    API Test Addfiles  --bbFFlag=0xff00 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}   8
 
    mpirun  rm -rf ${MOUNTPT}/masterfile
	 

API User cannot addfiles to Invalid BBTransferDef_t  
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --noTransferDef --bbFlag --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source/target file to a non-existent transfer definition
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M
    API Test Addfiles  --noTransferDef --bbFFlag=0 --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}   8
    mpirun  rm -rf ${MOUNTPT}/masterfile
	 


API BB_AddFiles - duplicate source files can be added to transfer definition where target is Directory
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source file to a transfer def with target as Directory
    ...		          ... Also attempt to add the same source to directory
    ...		     EXPECT:   Success 
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M

    API Test Addfiles  --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
    API Test Addfiles  --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}
     
    mpirun  rm -rf ${MOUNTPT}/masterfile


API BB_AddFiles - Basic File to File Transfer 
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --srcFile --targetFile 
    ...  	     ERROR: Attempt to add a source file to a transfer def with target as file 
    ...		     EXPECT:   Success 
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M

    API Test Addfiles  --srcFile=${MOUNTPT}/masterfile --targetFile=${PFSDIR}/target-mstrfl
     
    mpirun  rm -rf ${MOUNTPT}/masterfile
    ${rc}=  Run and Return RC  rm -rf ${PFSDIR}/target-mstrfl
    Should be equal as integers  ${rc}  0

API BB_AddFiles - Basic File to File Transfer - cannot add two files to same target file 
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --srcFile --srcFile2--targetFile 
    ...  	     ERROR: Attempt to add a two source files to a transfer def with target as file 
    ...		     EXPECT:   FAILURE 
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile2 bs=1 count=1M

    API Test Addfiles  --srcFile=${MOUNTPT}/masterfile --srcFile2=${MOUNTPT}/masterfile2 --targetFile=${PFSDIR}/target-mstrfl   255
     
    mpirun  rm -rf ${MOUNTPT}/masterfile
    mpirun  rm -rf ${MOUNTPT}/masterfile2


API BB_AddFiles - Basic File to Directory Transfer - One or more files can be added to Directory 
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Testcase:   test_addfiles --srcFile --srcFile2--targetFile 
    ...  	     ERROR: Attempt to add a two source files to a transfer def with target as Directory 
    ...		     EXPECT:  SUCCESS 
    
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile bs=1 count=1M
    mpirun  dd if=/dev/urandom of=${MOUNTPT}/masterfile2 bs=1 count=1M

    API Test Addfiles  --srcFile=${MOUNTPT}/masterfile --srcFile2=${MOUNTPT}/masterfile2 --targetFile=${PFSDIR}
     
    mpirun  rm -rf ${MOUNTPT}/masterfile
    mpirun  rm -rf ${MOUNTPT}/masterfile2

API uninitialized transfer definition fails BB_AddFiles
    [Tags]  fvt  addfiles
    [Documentation]  
    ...		     Call addfiles4.c testcase
    ...		     ERROR: trasferDef was never created
    
    mpirun  ${RANDFILE} --file ${MOUNTPT}/file
    mpirun  ${WORKDIR}/bb/tests/bin/addfiles4 ${MOUNTPT}/file ${PFSDIR}/file


API GetTransferHandle Test - Single for BBTAG 
    [Tags]  fvt
    [Documentation]  
    ...		     Call getTransferHandle2.c testcase.  
    ...		     Generate a transfer handle for a given BBTAG, for a single contributor.  
    
    API Test Addfiles  --contribSz=1 --getTransferHdl=22 

API GetTransferHandle Test - Duplicate Single for BBTAG 
    [Tags]  fvt
    [Documentation]  
    ...		     Call getTransferHandle2.c testcase.  
    ...		     Generate a *duplicate* transfer handle for a given BBTAG, for a single contributor.  
    
    API Test Addfiles  --contribSz=1 --getDupTransferHdl=22 

API GetTransferHandle Test - Single Multi Contributor for BBTAG 
    [Tags]  fvt
    [Documentation]  
    ...		     Call getTransferHandle2.c testcase.  
    ...		     Generate a transfer handle for a given BBTAG, for a multiple contributors.  
    
    API Test Addfiles  --contribSz=20 --getTransferHdl=22 

API GetTransferHandle Test - Duplicate Multi Contributor for BBTAG 
    [Tags]  fvt
    [Documentation]  
    ...		     Call getTransferHandle2.c testcase.  
    ...		     Generate a *duplicate* transfer handle for a given BBTAG, for a multiple contributors.  
    
    API Test Addfiles  --contribSz=20 --getDupTransferHdl=22 


User can transfer file to SSD whereby filename contains shell characters1
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  \!@#'  quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "\!@#'"  22

User can transfer file to SSD whereby filename contains shell characters2
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  $;?"*  quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   '$;?"*'  -1

User can transfer file to SSD whereby filename contains shell characters3
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  &()%  quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "&()%"  -1

User can transfer file to SSD whereby filename contains shell characters4
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  []{} quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "[]{}"  0


User can transfer file to SSD whereby filename contains shell characters5
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  :<>+=  quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   ":<>+="  -1

User can transfer file to SSD whereby filename contains shell characters6
     [Tags]  unittest  transfer  tossd  
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  -~|  quotes ...

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "-~|"  -1



User can transfer file from SSD whereby filename contains shell characters1
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  \!@# quotes ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "\!@#'"  22

User can transfer file from SSD whereby filename contains shell characters2
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  &()%  ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "&()%"  -1
   
User can transfer file from SSD whereby filename contains shell characters3
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  [].{}  quotes ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "[].{}"  0

User can transfer file from SSD whereby filename contains shell characters4
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  :<>+=  quotes ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   ":<>+="  -1

User can transfer file from SSD whereby filename contains shell characters5
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  -~|  quotes ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   "-~|"  -1

User can transfer file from SSD whereby filename contains shell characters6
     [Tags]  unittest  transfer  fromssd
     [Documentation]
     ...		Perform a normal file transfer, however the source filename contains shell characters
     ...		Shell characters are like:  $;?"*  quotes ...

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run parameterized transfer with Special Chars   '$;?"*'  -1

User can transfer files local-local
    [Tags]  unittest  transfer  tossd
     set test variable  ${source}    ${MOUNTPT}/source
     set test variable  ${dest}      ${MOUNTPT}/target
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Create directory   ${MOUNTPT}/source
     Create directory   ${MOUNTPT}/target
     Run parameterized transfer

User can transfer files remote-remote
    [Tags]  unittest  transfer  tossd
     set test variable  ${source}    ${PFSDIR}/source
     set test variable  ${dest}      ${PFSDIR}/target
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576
     Run Keyword And Continue On Failure  OperatingSystem.Create directory   ${PFSDIR}/source
     Run Keyword And Continue On Failure  OperatingSystem.Create directory   ${PFSDIR}/target
     Run parameterized transfer


*** Keywords ***
API Test Addfiles  
     [Arguments]    ${addfiles_parms}   ${expect_rc}=0
     mpirun  ${WORKDIR}/bb/tests/bin/test_addfiles ${addfiles_parms}   ${expect_rc}

API Test Transfer_rate  
     [Arguments]    ${transfer_parms}   ${expect_rc}=0
     mpirun  ${WORKDIR}/bb/tests/bin/transfer_rate ${transfer_parms}   ${expect_rc}


