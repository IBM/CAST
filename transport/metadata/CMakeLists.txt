file(GLOB pickle_files *.[p|txt])
install(FILES ${pickle_files} COMPONENT transport-devel DESTINATION transport/metadata)
