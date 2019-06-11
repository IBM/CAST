*** Settings ***
Resource        ../common.robot
Test Setup      Setup LSF testcases
Suite setup     Clear LSF jobs
Suite teardown  Clear LSF jobs

*** Test Cases ***

Sanity check LSF+CSM configuration
    [Tags]  lsf
    [Timeout]  5 minute
    bsub&wait  hostname

Sanity check LSF+CSM+JSM configuration
    [Tags]  lsf
    [Timeout]  1 minute
    bsub&wait  ${jsrun} hostname

Sanity check LSF+CSM configuration error path
    [Tags]  lsf
    [Timeout]  5 minute
    bsub&wait  badcommandshouldfail  127

Sanity check LSF+CSM+JSM configuration error path
    [Tags]  lsf
    [Timeout]  1 minute
    bsub&wait  ${jsrun} badcommandshouldfail  210

Sanity check LSF+CSM+JSM configuration multi-node
    [Tags]  lsf
    [Timeout]  1 minute
    set num computes  2
    bsub&wait  ${jsrun} hostname

Sanity check LSF+CSM+JSM configuration under queue load
     [Tags]  lsf
     :FOR  ${i}   in range   1   10
     \  bsub  job${i}  ${jsrun} hostname
     
     :FOR  ${i}   in range   1   10
     \  waitproc  job${i}  0
