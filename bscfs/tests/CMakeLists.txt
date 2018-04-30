find_path(MPI_BIN NAMES mpicc
	  PATHS /opt/ibm/spectrum_mpi/bin /lib64/openmpi/bin)
IF(MPI_BIN)

message("Using MPI found in ${MPI_BIN}")
set(MPI_C_COMPILER ${MPI_BIN}/mpicc)
set(MPI_CXX_COMPILER ${MPI_BIN}/mpicxx)
find_package(MPI REQUIRED)

add_executable(chkpnt_write chkpnt_write.c)
add_dependencies(chkpnt_write need_bbapi_version)
target_include_directories(chkpnt_write PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_write ${MPI_C_LIBRARIES})
install(TARGETS chkpnt_write COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_read chkpnt_read.c)
add_dependencies(chkpnt_read need_bbapi_version)
target_include_directories(chkpnt_read PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_read ${MPI_C_LIBRARIES})
install(TARGETS chkpnt_read COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_combo chkpnt_combo.c)
add_dependencies(chkpnt_combo need_bbapi_version)
target_include_directories(chkpnt_combo PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_combo ${MPI_C_LIBRARIES})
install(TARGETS chkpnt_combo COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_write_PFS chkpnt_write_PFS.c)
add_dependencies(chkpnt_write_PFS need_bbapi_version)
target_include_directories(chkpnt_write_PFS PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_write_PFS ${MPI_C_LIBRARIES})
install(TARGETS chkpnt_write_PFS COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_read_PFS chkpnt_read_PFS.c)
add_dependencies(chkpnt_read_PFS need_bbapi_version)
target_include_directories(chkpnt_read_PFS PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_read_PFS ${MPI_C_LIBRARIES})
install(TARGETS chkpnt_read_PFS COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_write_multi chkpnt_write_multi.c)
add_dependencies(chkpnt_write_multi need_bbapi_version)
target_include_directories(chkpnt_write_multi PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_write_multi ${MPI_C_LIBRARIES})
target_link_libraries(chkpnt_write_multi -lpthread)
install(TARGETS chkpnt_write_multi COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(chkpnt_read_multi chkpnt_read_multi.c)
add_dependencies(chkpnt_read_multi need_bbapi_version)
target_include_directories(chkpnt_read_multi PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(chkpnt_read_multi ${MPI_C_LIBRARIES})
target_link_libraries(chkpnt_read_multi -lpthread)
install(TARGETS chkpnt_read_multi COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

install(PROGRAMS chkpnt_cleanup COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

install(PROGRAMS run_chkpnt_write COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

install(PROGRAMS run_chkpnt_read COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

install(PROGRAMS lsfrun_chkpnt_write COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

install(PROGRAMS lsfrun_chkpnt_read COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

ELSE(MPI_BIN)
    message(WARNING "MPI not found; skipping MPI-based BSCFS tests")
ENDIF(MPI_BIN)

add_executable(gen_internal_files gen_internal_files.c ../tools/MapfileUtil.c)
install(TARGETS gen_internal_files COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(install_internal_files install_internal_files.c)
add_dependencies(install_internal_files need_bbapi_version)
install(TARGETS install_internal_files COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)

add_executable(flush flush.c)
add_dependencies(flush need_bbapi_version)
install(TARGETS flush COMPONENT burstbuffer-tests
	DESTINATION bscfs/tests)
