find_path(MPI_BIN NAMES mpicc
          PATHS /opt/ibm/spectrum_mpi/bin /lib64/openmpi/bin)

add_compile_options("-O2")

IF(MPI_BIN)
message("Using MPI found in ${MPI_BIN}")
set(MPI_C_COMPILER ${MPI_BIN}/mpicc)
set(MPI_CXX_COMPILER ${MPI_BIN}/mpicxx)
find_package(MPI REQUIRED)

add_executable(randfile-mpi randfile.cc)
install(TARGETS randfile-mpi  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_include_directories(randfile-mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(randfile-mpi ${Boost_LIBRARIES} ${MPI_C_LIBRARIES})
target_compile_definitions(randfile-mpi PRIVATE -DUSE_MPI=1 -DUSE_SC_LOGGER=1)

add_executable(comparefiles-mpi comparefiles.cc)
install(TARGETS comparefiles-mpi  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_include_directories(comparefiles-mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(comparefiles-mpi fsutil ${Boost_LIBRARIES} ${MPI_C_LIBRARIES})
target_compile_definitions(comparefiles-mpi PRIVATE -DUSE_MPI=1 -DUSE_SC_LOGGER=1)

add_executable(removefiles-mpi removefiles.cc)
install(TARGETS removefiles-mpi  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_include_directories(removefiles-mpi PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(removefiles-mpi fsutil ${Boost_LIBRARIES} ${MPI_C_LIBRARIES})
target_compile_definitions(removefiles-mpi PRIVATE -DUSE_MPI=1 -DUSE_SC_LOGGER=1)

ENDIF(MPI_BIN)


add_executable(randfile randfile.cc)
install(TARGETS randfile  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_link_libraries(randfile ${Boost_LIBRARIES})
target_compile_definitions(randfile PRIVATE -DUSE_SC_LOGGER=1)

add_executable(comparefiles comparefiles.cc)
install(TARGETS comparefiles  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_link_libraries(comparefiles fsutil ${Boost_LIBRARIES})
target_compile_definitions(comparefiles PRIVATE -DUSE_SC_LOGGER=1)

add_executable(removefiles removefiles.cc)
install(TARGETS removefiles  COMPONENT burstbuffer-tools DESTINATION bb/tools)
target_link_libraries(removefiles fsutil ${Boost_LIBRARIES})
target_compile_definitions(removefiles PRIVATE -DUSE_SC_LOGGER=1)


install(FILES scripts/analyze.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
install(FILES scripts/common.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
install(FILES scripts/generateErrorsListing.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
install(FILES scripts/generateWorkQueueMgrDumps.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
install(FILES scripts/generateTransferRates.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
install(FILES scripts/processConsoleLogs.py COMPONENT burstbuffer-tools DESTINATION bb/tools/scripts)
