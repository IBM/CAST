add_executable(MapfileDisplay MapfileDisplay.c MapfileUtil.c)
add_executable(MapfileCreate MapfileCreate.c MapfileUtil.c)

install(TARGETS MapfileDisplay COMPONENT burstbuffer DESTINATION bscfs/tools)
install(TARGETS MapfileCreate  COMPONENT burstbuffer DESTINATION bscfs/tools)
