add_custom_command(
        OUTPUT bb.cfg
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genbbcfg.pl ${CMAKE_CURRENT_SOURCE_DIR}/bb.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/bb.cfg ${RPMBUILD}
        COMMENT "Generating bb.cfg"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bb.cfg.in ${CMAKE_CURRENT_SOURCE_DIR}/genbbcfg.pl
        )
add_custom_target(need_bbcfg ALL DEPENDS bb.cfg)


add_executable(bbstagein_admin stage-admin-wrapper.c)
target_compile_definitions(bbstagein_admin PRIVATE -DSCRIPTPATH=\"${CMAKE_INSTALL_PREFIX}/bb/scripts/stagein_admin.pl\")

add_executable(bbstageout_admin stage-admin-wrapper.c)
target_compile_definitions(bbstageout_admin PRIVATE -DSCRIPTPATH=\"${CMAKE_INSTALL_PREFIX}/bb/scripts/stageout_admin.pl\")

# Do the install twice...  needed for setuid for some reason (cmake bug?)
install(TARGETS bbstagein_admin  COMPONENT burstbuffer DESTINATION bb/sbin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID)
install(TARGETS bbstagein_admin  COMPONENT burstbuffer DESTINATION bb/sbin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID)

# Do the install twice...  needed for setuid for some reason (cmake bug?)
install(TARGETS bbstageout_admin  COMPONENT burstbuffer DESTINATION bb/sbin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID)
install(TARGETS bbstageout_admin  COMPONENT burstbuffer DESTINATION bb/sbin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID)

# Only install to /usr/lib/systemd/system if building an rpm for distribution
# Otherwise ship to the work directory for test purposes
if(RPMBUILD)
  install(FILES bbproxy.service bbserver.service bbhealth.service COMPONENT burstbuffer DESTINATION /usr/lib/systemd/system)
else()
  install(FILES bbproxy.service bbserver.service bbhealth.service COMPONENT burstbuffer DESTINATION usr/lib/systemd/system)
endif()

install(PROGRAMS esub.bb  COMPONENT burstbuffer-lsf DESTINATION bb/scripts)
install(PROGRAMS epsub.bb COMPONENT burstbuffer-lsf DESTINATION bb/scripts)
install(PROGRAMS bb_pre_exec.sh  COMPONENT burstbuffer-lsf DESTINATION bb/scripts)
install(PROGRAMS bb_post_exec.sh COMPONENT burstbuffer-lsf DESTINATION bb/scripts)


install(PROGRAMS fakelsf.sh COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stagein_admin.pl  COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stageout_admin.pl COMPONENT burstbuffer DESTINATION bb/scripts)
install(FILES bbtools.pm COMPONENT burstbuffer DESTINATION bb/scripts)

install(PROGRAMS stagein_user.pl  COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stageout_user_phase1.pl COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stageout_user_phase2.pl COMPONENT burstbuffer DESTINATION bb/scripts)

install(PROGRAMS stagein_user_bscfs.pl  COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stageout_user_phase1_bscfs.pl COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS stageout_user_phase2_bscfs.pl COMPONENT burstbuffer DESTINATION bb/scripts)

install(PROGRAMS launch_job.sh COMPONENT burstbuffer DESTINATION bb/scripts)

install(PROGRAMS BSCFS_start.pl COMPONENT burstbuffer DESTINATION bb/scripts RENAME BSCFS_start)
install(PROGRAMS BSCFS_stop.pl  COMPONENT burstbuffer DESTINATION bb/scripts RENAME BSCFS_stop)

install(PROGRAMS mkcertificate.sh  COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS bbactivate.pl     COMPONENT burstbuffer DESTINATION bb/scripts RENAME bbactivate)
install(PROGRAMS nvmfConnect.sh    COMPONENT burstbuffer DESTINATION bb/scripts)
install(FILES    nvmet.json        COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS mkbbvg.sh         COMPONENT burstbuffer DESTINATION bb/scripts)
install(PROGRAMS setServer.pl      COMPONENT burstbuffer DESTINATION bb/scripts RENAME setServer)
install(PROGRAMS bbhealth.pl      COMPONENT burstbuffer DESTINATION bb/scripts RENAME bbhealth)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bb.cfg COMPONENT burstbuffer DESTINATION bb/scripts)

if(RPMBUILD)
  install(DIRECTORY DESTINATION /bscfs COMPONENT burstbuffer DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)
endif()

build_pathsetup(burstbuffer bb/bin)

if(RPMBUILD)
  install(FILES nvme.conf COMPONENT burstbuffer DESTINATION /etc/modprobe.d)
  install(FILES    bb_stagein      COMPONENT burstbuffer-lsf DESTINATION /etc/logrotate.d)
  install(FILES    bb_stageout     COMPONENT burstbuffer-lsf DESTINATION /etc/logrotate.d)
else()
  install(FILES nvme.conf COMPONENT burstbuffer DESTINATION etc/modprobe.d)
  install(FILES    bb_stagein      COMPONENT burstbuffer-lsf DESTINATION etc/logrotate.d)
  install(FILES    bb_stageout     COMPONENT burstbuffer-lsf DESTINATION etc/logrotate.d)
endif()
