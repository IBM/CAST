
include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

# cmd line interface for diag_run_begin
add_executable(csm_diag_run_begin diag_run_begin.c)
install(TARGETS csm_diag_run_begin COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_diag_run_begin PRIVATE ./)
target_link_libraries(csm_diag_run_begin csmi csm_network_c csmutil)

# cmd line interface for diag_run_end
add_executable(csm_diag_run_end diag_run_end.c)
install(TARGETS csm_diag_run_end COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_diag_run_end PRIVATE ./)
target_link_libraries(csm_diag_run_end csmi csm_network_c csmutil)

# cmd line interface for diag_run_query
add_executable(csm_diag_run_query diag_run_query.c)
install(TARGETS csm_diag_run_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_diag_run_query PRIVATE ./)
target_link_libraries(csm_diag_run_query csmi csm_network_c csmutil fsutil)

# cmd line interface for diag_run_query_details
add_executable(csm_diag_run_query_details diag_run_query_details.c)
install(TARGETS csm_diag_run_query_details COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_diag_run_query_details PRIVATE ./)
target_link_libraries(csm_diag_run_query_details csmi csm_network_c csmutil)

# cmd line interface for diag_result_create
add_executable(csm_diag_result_create diag_result_create.c)
install(TARGETS csm_diag_result_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_diag_result_create PRIVATE ./)
target_link_libraries(csm_diag_result_create csmi csm_network_c csmutil)

add_definitions(-DUSE_SC_LOGGER)
