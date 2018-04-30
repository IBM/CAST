add_executable(testidentity testidentity.cc)

flightgen(testidentity testidentity_fl.h)
flightlib(testidentity fsutil)

target_link_libraries(testidentity fsutil -lpthread)
