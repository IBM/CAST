*** Variables ***
@{newlist}

*** Keywords ***

Setup resiliency environment
     Setup test environment
     Set Suite Variable  ${SUITEISROOT}  1
     Open connection  primary
     Activate connection  primary
     bbcmd  resume
     Set Suite Variable  ${SUITEISROOT}  0
     

Teardown resiliency environment
     Set Suite Variable  ${SUITEISROOT}  1
     Open connection  primary
     Activate connection  primary
     bbcmd  resume
     Set Suite Variable  ${SUITEISROOT}  0


Failover back to primary
     shellrun  sudo ${WORKDIR}/bb/scripts/setServer --server=primary --hosts=${HOSTLIST} --jobid=${JOBID}
     Restore rank list
     Cleanup transfer test

Setup LV
     Start job
     Run as root
     Setup logical volume  ${MOUNTPT}  64G
     Run as user
     set test variable  ${numiter}   1
     set test variable  ${maxfiles}  1
     set test variable  ${minsize}   1048576
     set test variable  ${maxsize}   1048576

Setup to SSD
     Setup LV
     set test variable  ${source}    ${PFSDIR}
     set test variable  ${dest}      ${MOUNTPT}

Setup from SSD
     Setup LV
     set test variable  ${source}    ${MOUNTPT}
     set test variable  ${dest}      ${PFSDIR}

Use hosts with defined server
    [Arguments]  ${server}
    bbcmd  getserver  --connected=${server}
    @{_tmp}=  Split String  ${HOSTLIST}  ,
    @{_hostlist}=  create list  @{_tmp}

    set test variable  @{_hostlist}
    set test variable  @{newlist}
    For all ranks  append host to list  unless  serverList  == 'none'

    ${tmp}=  Catenate  SEPARATOR=,  @{newlist}
    set test variable  ${HOSTLIST}  ${tmp}
    
    #  Regenerate ranks
    ${numranks} =  Get Length	  ${newlist}
    @{_ranklist} =  create list
    :FOR  ${rank}  in range  ${numranks}    
    \  append to list  ${_ranklist}  ${rank}
    
    ${tmp}=  Catenate  SEPARATOR=,  @{_ranklist}
    set test variable  ${RANKLIST}  ${tmp}
    Setup CMDLAUNCHER


Use ranks with defined server
    [Arguments]  ${server}
    bbcmd  getserver  --connected=${server}
    set test variable  @{newlist}
    For all ranks  append rank to list  unless  serverList  == 'none'
    Log  list: ${newlist}
    ${tmp}=  Catenate  SEPARATOR=,  @{newlist}
    set test variable  ${RANKLIST}  ${tmp}

Append rank to list
     [Arguments]  ${rank}  ${op}  ${value}  ${cond}
     ${tmp}=  read_device_data  ${value}  ${rank}
     run keyword if  '${op}' == 'unless'  run keyword unless  '${tmp}' ${cond}  Append To List  ${newlist}  ${rank}
     run keyword if  '${op}' == 'if'      run keyword if      '${tmp}' ${cond}  Append To List  ${newlist}  ${rank}
     Log  list: ${newlist}

Append host to list
     [Arguments]  ${rank}  ${op}  ${value}  ${cond}
     ${tmp}=  read_device_data  ${value}  ${rank}
     set test variable  ${v}  @{_hostlist}[${rank}]
     run keyword if  '${op}' == 'unless'  run keyword unless  '${tmp}' ${cond}  Append To List  ${newlist}  ${v}
     run keyword if  '${op}' == 'if'      run keyword if      '${tmp}' ${cond}  Append To List  ${newlist}  ${v}
     Log  list: ${newlist}
