*** Settings ***
Library       Process
Library       String


*** Keywords ***
Run parameterized transfer
    [Arguments]
    [Documentation]  Runs $numiter iterations of generating random files
     :FOR  ${tagid}  in range  ${numiter}
     \  Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     \  ${handle}=  Run a file transfer  ${tagid}  ${MOUNTPT}/filelist
     \  Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
     \  Transfer status is  ${handle}  BBFULLSUCCESS
     \  Compare Random files  ${MOUNTPT}/filelist
     \  Remove Random files   ${MOUNTPT}/filelist

Run parameterized transfer with Special Chars
    [Arguments]   ${spC}  ${expect_rc}=0
    [Documentation]  Runs $numiter iterations of generating random files
     :FOR  ${tagid}  in range  ${numiter}
     \  Generate File List with Special Chars   ${source}  ${dest}  ${MOUNTPT}/filelist  ${spC}
     \  ${handle}=  Run a file transfer  ${tagid}  ${MOUNTPT}/filelist  ${expect_rc}
     \  Run keyword if  ${expect_rc} == 0  Wait Transfer Complete and Check  ${handle}  ${MOUNTPT}/filelist
     \  Run keyword if  ${expect_rc} == 0  Compare Random files  ${MOUNTPT}/filelist
     \  Run keyword if  ${expect_rc} == 0  Remove Random files   ${MOUNTPT}/filelist
     \  Run Keyword And Continue On Failure  Remove Random files   ${MOUNTPT}/filelist

Wait Transfer Complete and Check
     [Arguments]  ${handle}  ${filelist}
     Wait Until Keyword Succeeds  ${polltimeout}  1 second  transfer has completed  ${handle}
     Transfer status is  ${handle}  BBFULLSUCCESS


Run a file transfer
    [Arguments]  ${tagid}  ${filelist}  ${expect_rc}=0
    ${handle}=  Get transfer handle  ${tagid}
    Start file transfer  ${handle}  ${MOUNTPT}/filelist  ${expect_rc}
    [return]  ${handle}

API_Run parameterized transfer
    [Arguments]
    [Documentation]  Runs $numiter iterations of generating random files
     :FOR  ${tagid}  in range  ${numiter}
     \  Generate File List  ${source}  ${dest}  ${MOUNTPT}/filelist
     \  API_Run a file transfer  ${MOUNTPT}/filelist
     \  Compare Random files  ${MOUNTPT}/filelist
     \  Remove Random files   ${MOUNTPT}/filelist

API_Run a file transfer
    [Arguments]  ${filelist}  ${expect_rc}=0
    API_Start file transfer  ${MOUNTPT}/filelist  ${expect_rc}

Cleanup transfer test
	[Arguments]
	Run Keyword And Continue On Failure  Remove Random files  ${MOUNTPT}/filelist
	Run Keyword And Continue On Failure  Teardown logical volume  ${MOUNTPT}
	Run Keyword And Continue On Failure  Remove directories

Generate File List
     [Arguments]  ${sourcepath}  ${targetpath}  ${filelist}
     mpirun  ${RANDFILE} --sourcepath ${sourcepath} --targetpath ${targetpath} --minfiles ${minfiles} --maxfiles ${maxfiles} --minsize ${minsize} --maxsize ${maxsize} --genfilelist --by 512 --file ${filelist}

Generate File List with Special Chars
     [Arguments]  ${sourcepath}  ${targetpath}  ${filelist}  ${spChars}
     mpirun  ${RANDFILE} --sourcepath ${sourcepath} --targetpath ${targetpath} --minfiles ${minfiles} --maxfiles ${maxfiles} --minsize ${minsize} --maxsize ${maxsize} --genfilelist --by 512 --specialChars ${spChars} --file ${filelist}

Remove Random files
     [Arguments]  ${filelist}
     mpirun  ${REMOVEFILES} --filelist ${filelist}

Compare Random files
     [Arguments]  ${filelist}
     Return from keyword if  '${SKIPCOMPARE}' == '1'
     mpirun  ${COMPAREFILES} --filelist ${filelist}

mpirun
     [Arguments]  ${cmdargs}  ${expect_rc}=0  ${cmdtimeout}=10m
     Log  Command: ${CMDLAUNCHER} ${cmdargs}
     @{cmd} =     Split Command Line    ${CMDLAUNCHER} ${cmdargs}
     ${result} =  Run Process   @{cmd}  timeout=${cmdtimeout}  on_timeout=kill
     Log  Output: ${result.stdout} ${result.stderr}
     Should be equal as integers  ${result.rc}  ${expect_rc}
     [Return]  ${result.stdout} ${result.stderr}

shellrun
     [Arguments]  ${cmdargs}  ${expect_rc}=0  ${cmdtimeout}=10m
     Log  Command: ${cmdargs}
     @{cmd} =     Split Command Line    ${cmdargs}
     ${result} =  Run Process   @{cmd}  timeout=${cmdtimeout}  on_timeout=kill
     Log  Output: ${result.stdout} ${result.stderr}
     Should be equal as integers  ${result.rc}  ${expect_rc}
     [Return]  ${result.stdout} ${result.stderr}

shellrun with cwd
     [Arguments]  ${cmdargs}  ${newcwd}  ${expect_rc}=0  ${cmdtimeout}=10m
     Log  Command: ${cmdargs}
     @{cmd} =     Split Command Line    ${cmdargs}
     ${result} =  Run Process   @{cmd}  timeout=${cmdtimeout}  on_timeout=kill  cwd=${newcwd}
     Log  Output: ${result.stdout} ${result.stderr}
     Should be equal as integers  ${result.rc}  ${expect_rc}
     [Return]  ${result.stdout} ${result.stderr}

forkproc
	[Arguments]  ${handle}  ${cmdargs}
        @{setupcmd} =  Split Command Line    ${cmdargs}
        Start Process   @{setupcmd}  alias=${handle}

waitproc
	[Arguments]  ${handle}  ${expect_rc}=0  ${timeout}=10min
	
	${result}=  Wait For Process	handle=${handle}  timeout=${timeout}  on_timeout=kill
	
	Log  ExitRC: ${result.rc}
	Log  Output: ${result.stdout} ${result.stderr}
	Should be equal as integers  ${result.rc}  ${expect_rc}
	[Return]  ${result}

ok to run mpi executable
     Run keyword if  '${HOSTLIST}' == ''  broken  Testcase runs an MPI executable but HOSTLIST is empty

Setup logical volume
    [Arguments]  ${path}  ${size}  ${expect_rc}=0
     ${prigroup}=  Get Primary Group  ${TESTUSER}
     Create directory  ${path}
     Change owner and group  ${path}  ${TESTUSER}  ${prigroup}
     Change mode  ${path}  0777
     Create logical volume  ${path}  ${size}  ${expect_rc}
     Change owner and group  ${path}  ${TESTUSER}  ${prigroup}
     Change mode  ${path}  0777

API_Setup logical volume
    [Arguments]  ${path}  ${size}  ${expect_rc}=0
     ${prigroup}=  Get Primary Group  ${TESTUSER}
     API_Create directory  ${path}
     API_Change owner and group  ${path}  ${TESTUSER}  ${prigroup}
     API_Change mode  ${path}  0777
     API_Create logical volume  -c -m ${path} -s ${size}  ${expect_rc}
     Append to list  ${MOUNTLVS}  ${path}
     API_Change owner and group  ${path}  ${TESTUSER}  ${prigroup}
     API_Change mode  ${path}  0777

Get Primary Group
    [Arguments]  ${user}
    ${rc}  ${primarygroup} =  Run and Return RC and Output  id -ng ${user}
    [return]  ${primarygroup}

Remove directories
    [Arguments]
    [Timeout]  30 seconds
    Run as root
    :FOR  ${path}  in  @{MOUNTDIRS}
    \  Run Keyword And Continue On Failure  bbcmd  remove  --mount=${path}
    \  bbcmd    rmdir    --path  ${path}
    \  Status should be  0
    \  Remove Values from List  ${MOUNTDIRS}  ${path}

Teardown Logical Volume
    [Arguments]  ${path}
    [Timeout]  600 seconds
    Run as root
    :FOR  ${path}  in  @{MOUNTLVS}
    \  bbcmd  remove  --mount=${path}
    \  Remove Values from List  ${MOUNTLVS}  ${path}
    Run Keyword And Continue On Failure  Remove directories
    bbcmd  removejobinfo

Transfer status is
    [Arguments]  ${handle}  ${expected_status}
    [Timeout]  120 seconds
    bbcmd  getstatus  --handle  ${handle}
    Status should be  0
    ${status} =  read transfer status  0
    Should be equal  ${expected_status}  ${status}

Transfer has completed
    [Arguments]  ${handle}
    [Timeout]  120 seconds
    bbcmd  getstatus  --handle  ${handle}
    Status should be  0
    ${status} =  read transfer status  0
    Should not be equal  ${status}  BBINPROGRESS
    Should not be equal  ${status}  BBNOTSTARTED
    Should not be equal  ${status}  BBSTOPPED

Verify block
    [Arguments]  ${mountpoint}   ${size}=0  ${writesize}=0
    mpirun  ${WORKDIR}/bb/tests/bin/verify_block.pl ${mountpoint} ${size} ${writesize}


My convert to Bytes
    [Arguments]  ${size}

     ${sz_return} =    Convert to integer  0
     ${blk_sz} =    Convert to integer  0
     ${K_Bytes} =   Convert to integer  1024
     ${M_Bytes} =   Evaluate  ${K_Bytes} * ${K_Bytes}
     ${G_Bytes} =   Evaluate  ${M_Bytes} * ${K_Bytes}
     ${T_Bytes} =   Evaluate  ${G_Bytes} * ${K_Bytes}
     ${P_Bytes} =   Evaluate  ${T_Bytes} * ${K_Bytes}


     ${rc}  ${token} =  Run and Return RC and Output  echo ${size} | cut -c1-1
     ${rc}  ${l_size} =  Run keyword if   '${token}' == '+' or '${token}' == '-'  Run and Return RC and Output  echo ${size} | awk '{print substr($0,2)}'
     ${size} =  Run keyword if  '${l_size}' == 'None' or '${l_size}' == ''  Set variable  ${size}  ELSE  Set variable  ${l_size}


     ${rc}  ${blk_sz} =  Run keyword If  "${size}" != "None" and "${size}" != ""  Run and Return RC and Output  echo ${size} | rev | cut -c 2- | rev
     ${rc}  ${unit} =   Run keyword If  "${size}" != "None" and "${size}" != ""   Run and Return RC and Output  echo ${size} | awk '{print substr($0,length,1)}'

     ${sz_return} =  Run keyword if  "${unit}" == "M"  Evaluate  ${blk_sz} * ${M_Bytes}  ELSE IF  "${unit}" == "G"  Evaluate  ${blk_sz} * ${G_Bytes}  ELSE IF  "${unit}" == "T"  Evaluate  ${blk_sz} * ${T_Bytes}  ELSE IF  "${unit}" == "P"  Evaluate  ${blk_sz} * ${P_Bytes}  ELSE  Set variable If  "${size}" != "None" and "${size}" != ""   ${size}

    [return]  ${sz_return}

********* UNUSED ************
Get Random
    [Arguments]  ${lower}  ${upper}
    ${value}=  Evaluate  random.randint(${lower}, ${upper})   modules=random,sys
    [return]  ${value}


Files should compare
     [Arguments]  ${file1}  ${file2}
     ${rc}  ${hex1out} =  Run and Return RC and Output  hexdump -n 1024 ${file1}
     ${rc}  ${hex2out} =  Run and Return RC and Output  hexdump -n 1024 ${file2}
     ${rc}  ${hex2out} =  Run and Return RC and Output  ls -l ${file1}
     ${rc}  ${hex2out} =  Run and Return RC and Output  ls -l ${file2}
     ${rc}  ${hex2out} =  Run and Return RC and Output  cmp ${file1} ${file2}
     ${rc}  ${file1out} =  Run and Return RC and Output  md5sum ${file1} | cut -d " " -f 1
     Should be equal as integers  ${rc}  0
     ${rc}  ${file2out} =  Run and Return RC and Output  md5sum ${file2} | cut -d " " -f 1
     Should be equal as integers  ${rc}  0
     Should be equal  ${file1out}  ${file2out}

