*** Settings ***
Resource          ../common.robot
Suite Setup       setup test environment


*** Test Cases ***
User can copy files
     [Tags]  fvt
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  25 
     set test variable  ${minsize}   1024 
     Run parameterized transfer
     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
     ...   AND  Teardown logical volume  ${MOUNTPT}

User can get transfers
     [Tags]  fvt
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  10 
     set test variable  ${minsize}   1024 
     Run parameterized transfer
     @{handles}=  Get transfer handle list  
     FOR  ${handle}  IN  @{handles}
          Transfer status is  ${handle}   BBFULLSUCCESS
          Transfer has completed   ${handle}
     END
     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
     ...   AND  Teardown logical volume  ${MOUNTPT}

Verify device usage info 
     [Tags]  fvt  usage
     Start job
     Run as root

     [Timeout]  60 seconds
     bbcmd  getdeviceusage 
     Status should be  0 
     ${bytes_read_prior}=  read device data  data_read  0
     ${bytes_written_prior}=  read device data  data_written  0

     Setup logical volume  ${MOUNTPT}  16G
     Teardown logical volume  ${MOUNTPT}
     
     bbcmd  getdeviceusage 
     Status should be  0 
     ${bytes_read_after}=  read device data  data_read  0
     ${bytes_written_after}=  read device data  data_written  0
     ${temp}=  read device data  temperature  0

     Should not be true  ${bytes_read_after} <= ${bytes_read_prior}
     Should not be true  ${bytes_written_after} <= ${bytes_written_prior}
     Should not be true  ${temp} <= 10

Verify Transfer usage info for read
     [Tags]  fvt  usage
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

     bbcmd  getusage  --mount=${MOUNTPT}
     ${bytes_read_prior}=     read device data  burstBytesRead     0
     ${bytes_written_prior}=  read device data  burstBytesWritten  0

     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  10 
     set test variable  ${minsize}   1024 
     Run parameterized transfer
     @{handles}=  Get transfer handle list  
     FOR  ${handle}  IN  @{handles}
          Transfer status is  ${handle}   BBFULLSUCCESS
          Transfer has completed   ${handle}
     END
     
     bbcmd  getusage  --mount=${MOUNTPT}
     ${bytes_read_after}=     read device data  burstBytesRead     0
     ${bytes_written_after}=  read device data  burstBytesWritten  0
     
     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
     ...   AND  Teardown logical volume  ${MOUNTPT}
     
     Should be true      ${bytes_read_after} == ${bytes_read_prior}
     Should not be true  ${bytes_written_after} <= ${bytes_written_prior}

Verify Transfer usage info for write
     [Tags]  fvt  usage
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  16G
     Run as user

     bbcmd  getusage  --mount=${MOUNTPT}
     ${bytes_read_prior}=     read device data  burstBytesRead     0
     ${bytes_written_prior}=  read device data  burstBytesWritten  0

     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  10 
     set test variable  ${minsize}   1024 
     Run parameterized transfer
     @{handles}=  Get transfer handle list  
     FOR  ${handle}  IN  @{handles}
          Transfer status is  ${handle}   BBFULLSUCCESS
          Transfer has completed   ${handle}
     END
     
     bbcmd  getusage  --mount=${MOUNTPT}
     ${bytes_read_after}=     read device data  burstBytesRead     0
     ${bytes_written_after}=  read device data  burstBytesWritten  0
     
     Teardown logical volume  ${MOUNTPT}
     [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
     ...   AND  Teardown logical volume  ${MOUNTPT}
     
     Should be true      ${bytes_written_after} == ${bytes_written_prior}
     Should not be true  ${bytes_read_after} <= ${bytes_read_prior}

Verify set usage limit for write
    [Tags]  fvt  usage  todo
    Start job
    Run as root
    Setup logical volume  ${MOUNTPT}  2G
    Run as user

    bbcmd  getusage  --mount=${MOUNTPT}
    ${threshold_max_read}=  read device data  totalBytesRead  0
    ${threshold_max_write}=  read device data  totalBytesWritten  0
    ${threshold_max_read}=  Evaluate  ${threshold_max_read}-100
    ${threshold_max_write}=  Evaluate  ${threshold_max_write}-100

    Check write threshold   ${threshold_max_write}  ${MOUNTPT}

    Teardown logical volume  ${MOUNTPT}
    [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
    ...   AND  Teardown logical volume  ${MOUNTPT}

Verify set usage limit for read
    [Tags]  fvt  usage  todo
    Start job
    Run as root
    Setup logical volume  ${MOUNTPT}  2G
    Run as user

    bbcmd  getusage  --mount=${MOUNTPT}
    ${threshold_max_read}=  read device data  totalBytesRead  0
    ${threshold_max_write}=  read device data  totalBytesWritten  0
    ${threshold_max_read}=  Evaluate  ${threshold_max_read}-100
    ${threshold_max_write}=  Evaluate  ${threshold_max_write}-100

    Check read threshold   ${threshold_max_read}  ${MOUNTPT}
    
    Teardown logical volume  ${MOUNTPT}
    [TEARDOWN]  Run Keywords  Remove Random files   ${MOUNTPT}
    ...   AND  Teardown logical volume  ${MOUNTPT}

