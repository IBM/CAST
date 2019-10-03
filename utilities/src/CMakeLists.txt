add_library(fsutil string_tools.c crc.cc identity.cc util.cc Thread.cc logging.cc string_tokenizer.cc)
target_compile_definitions(fsutil PRIVATE -DUSE_SC_LOGGER=1)
target_link_libraries(fsutil -luuid -lpthread ${Boost_LIBRARIES})
flightgen(fsutil fsutil_flightlog.h)
