*** Keywords ***
RunPyTest
    [Arguments]  ${testcase}  ${args}
    set test variable  ${oldcmd}  ${CMDLAUNCHER}
    set test variable  ${CMDLAUNCHER}  python
    mpirun  ${WORKDIR}/bb/wrappers/bbapi_main.py --libpath ${WORKDIR}/bb/lib --testpath ${WORKDIR}/bb/wrappers --testcase ${testcase} ${args}
    set test variable  ${CMDLAUNCHER}  ${oldcmd}
