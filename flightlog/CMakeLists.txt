add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)

#### RPM ####
set(CPACK_RPM_flightlog_PACKAGE_SUMMARY           "Lightweight circular in-memory event log" PARENT_SCOPE)
set(CPACK_RPM_flightlog_PACKAGE_DESCRIPTION       "This RPM provides decoder and library to support in-memory flightlog.  Each flightlog entry is timestampped with a high-precision time, CPU#, and event specific information." PARENT_SCOPE)


#### RPM ####
set(CPACK_RPM_flightlog-devel_PACKAGE_SUMMARY           "Flightlog headers and build tools" PARENT_SCOPE)
set(CPACK_RPM_flightlog-devel_PACKAGE_DESCRIPTION       "This RPM provides developer header files and necessary build tools to construct flightlog instrumentation." PARENT_SCOPE)
