*** Keywords ***
RunPyTest
    [Arguments]  ${testcase}  ${args}
    set test variable  ${oldcmd}  ${CMDLAUNCHER}
    ${pysize}=  Get File Size  /usr/bin/python2

    Run keyword if      '${pysize} > 0'  set test variable  ${CMDLAUNCHER}  python2
    Run keyword unless  '${pysize} > 0'  set test variable  ${CMDLAUNCHER}  python

    mpirun  ${WORKDIR}/bb/wrappers/bbapi_main.py --libpath ${WORKDIR}/bb/lib --testpath ${WORKDIR}/bb/wrappers --testcase ${testcase} ${args}
    set test variable  ${CMDLAUNCHER}  ${oldcmd}
