#####################################################
# define and add test sources
#
set(FLIGHTLOG_TEST_SOURCES
  example.c
)

foreach(_test ${FLIGHTLOG_TEST_SOURCES})
  get_filename_component(TEST_NAME ${_test} NAME_WE)
  add_executable(${TEST_NAME} ${_test})
  flightgen(${TEST_NAME} ${TEST_NAME}_flightlog.h)
  
  add_test(FlightlogTest_${TEST_NAME} ${TEST_NAME})
endforeach()
