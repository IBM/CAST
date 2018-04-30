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

User can random transfer large files to SSD
     [Tags]  unittest  transfer  tossd  random  large
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   25
     set test variable  ${maxfiles}  4
     set test variable  ${minsize}   1024
     Run parameterized transfer


User can random transfer large files from SSD
     [Tags]  unittest  transfer  fromssd  random  large
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   25
     set test variable  ${maxfiles}  4
     set test variable  ${minsize}   1024
     Run parameterized transfer


User can random transfer small files to SSD
     [Tags]  unittest  transfer  tossd  random  small
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}
     set test variable  ${numiter}   25
     set test variable  ${maxfiles}  4
     set test variable  ${maxsize}   1024
     Run parameterized transfer


User can random transfer small files from SSD
     [Tags]  unittest  transfer  fromssd  random  small
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}
     set test variable  ${numiter}   25
     set test variable  ${maxfiles}  4
     set test variable  ${maxsize}   1024
     Run parameterized transfer
