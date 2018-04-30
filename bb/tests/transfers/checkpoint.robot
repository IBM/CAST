*** Settings ***
Resource 	  ../common.robot
Suite Setup       Setup test environment
Test Setup        Setup testcases
Test Teardown     Cleanup transfer test

*** Keywords ***
Setup Testcases
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  400G
     Run as user


*** Test Cases ***

User can transfer large file from SSD to NULL device
     [Tags]  unittest  transfer  fromssd  devnull  sw37
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      /dev/null
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     set test variable  ${SKIPCOMPARE}  1
     Run parameterized transfer

User can transfer large file to SSD from NULL device
     [Tags]  unittest  transfer  tossd  devnull  sw37
     set test variable  ${source}    /dev/zero
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     set test variable  ${SKIPCOMPARE}  1
     Run parameterized transfer

User can transfer large file from SSD to NULL device with rate limit
     [Tags]  unittest  transfer  fromssd  devnull  ratelimit  sw37
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      /dev/null
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     set test variable  ${SKIPCOMPARE}  1
     Set Logical Volume Throttle Rate  ${MOUNTPT}  536870912
     Run parameterized transfer

User can transfer large file to SSD from NULL device with rate limit
     [Tags]  unittest  transfer  tossd  devnull  ratelimit  sw37
     set test variable  ${source}    /dev/zero
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     set test variable  ${SKIPCOMPARE}  1
     Set Logical Volume Throttle Rate  ${MOUNTPT}  536870912
     Run parameterized transfer
     
User can transfer large file from SSD
     [Tags]  unittest  transfer  fromssd  sw37
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     Run parameterized transfer

User can transfer large file to SSD
     [Tags]  unittest  transfer  tossd  sw37
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   5
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   ${LARGEFILESIZE}
     set test variable  ${maxsize}   ${LARGEFILESIZE}
     Run parameterized transfer

User can perform small checkpoints to PFS
      [Tags]  unittest  transfer  fromssd  small  scrlike  sw37
      [Timeout]  300 seconds
      ok to run mpi executable
      :FOR  ${tagid}  in range  5
      \  Start job step
      \  Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      \  OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m 512 -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      \  Append To File  scrlike.out  ${result}
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1

User can perform large checkpoint to PFS
      [Tags]  unittest  transfer  fromssd  large  scrlike  sw37
      [Timeout]  3600 seconds
      ok to run mpi executable
      ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
      :FOR  ${tagid}  in range  5
      \  Start job step
      \  Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      \  OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      \  Append To File  scrlike.out  ${result}
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1

User can perform small checkpoint restore from PFS
      [Tags]  unittest  transfer  tossd  small  scrlike  sw37
      [Timeout]  3600 seconds
      ok to run mpi executable
      Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m 512 -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      
      :FOR  ${tagid}  in range  5
      \  Start job step
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m 512 -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 1 -w 1
      \  Append To File  scrlike.out  ${result}

User can perform large checkpoint restore from PFS
      [Tags]  unittest  transfer  tossd  large  scrlike  sw37
      [Timeout]  3600 seconds
      ok to run mpi executable
      ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
      Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      
      :FOR  ${tagid}  in range  5
      \  Start job step
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 1 -w 1
      \  Append To File  scrlike.out  ${result}


User can perform small overlapping checkpoints to PFS
      [Tags]  unittest  transfer  fromssd  scrlike
      [Timeout]  300 seconds
      ok to run mpi executable
      Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m 512 -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 5

User can perform large overlapping checkpoint to PFS
      [Tags]  unittest  transfer  fromssd  large  scrlike
      [Timeout]  1200 seconds
      ok to run mpi executable
      ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
      Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 5

User can checkpoint-to and restore-from a PFS
     [Tags]  unittest  transfer  fromssd  tossd  large  scrlike
     [Timeout]  1200 seconds
     ok to run mpi executable
     ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
     Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
     OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
     mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 1 -w 2

User can restore checkpoint PFS after successive checkpoints
     [Tags]  unittest  transfer  fromssd  tossd  large  scrlike
     [Timeout]  1200 seconds
     ok to run mpi executable
     ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
     Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
     OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
     mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 3 -w 4


User can perform large checkpoint to PFS without bbapi
      [Tags]  unittest  transfer  fromssd  large  scrlike  sw37  nobbapi  nofvt
      [Timeout]  3600 seconds
      ok to run mpi executable
      ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
      :FOR  ${tagid}  in range  5
      \  Start job step
      \  Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      \  OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -b -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      \  Append To File  scrlike2.out  ${result}
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1

User can perform large checkpoint restore from PFS without bbapi
      [Tags]  unittest  transfer  tossd  large  scrlike  sw37  nobbapi  nofvt
      [Timeout]  3600 seconds
      ok to run mpi executable
      ${checksize} =   Evaluate  ${LARGEFILESIZE}/8
      Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -b -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1

      :FOR  ${tagid}  in range  5
      \  Start job step
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -b -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 1 -w 1
      \  Append To File  scrlike3.out  ${result}


Check scaling of checkpoint file size
      [Tags]  unittest  transfer  fromssd  large  scrlike  sw37
      [Timeout]  3600 seconds
      ok to run mpi executable
      :FOR  ${tagid}  in range  17
      \  Start job step
      \  ${checksize} =   Evaluate  65536 * 2**${tagid}
      \  Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      \  OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      \  Append To File  scrlike4.out  ${result}
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1


Check scaling of checkpoint file size without bbapi
      [Tags]  unittest  transfer  fromssd  large  scrlike  sw37  nobbapi  nofvt
      [Timeout]  3600 seconds
      ok to run mpi executable
      :FOR  ${tagid}  in range  17
      \  Start job step
      \  ${checksize} =   Evaluate  65536 * 2**${tagid}
      \  Run Keyword And Continue On Failure  Remove Directory  ${PFSDIR}/ckpttest  recursive=yes
      \  OperatingSystem.Create Directory  ${PFSDIR}/ckpttest
      \  ${result}=  mpirun  ${WORKDIR}/bb/tests/bin/test_ckpt_mpi -b -c -m ${checksize} -l ${MOUNTPT} -p ${PFSDIR}/ckpttest -i ${JOBID} -r 0 -w 1
      \  Append To File  scrlike5.out  ${result}
      \  mpirun  rm -rf ${MOUNTPT}/ckpt_wave1
