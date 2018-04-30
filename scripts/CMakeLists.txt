install(PROGRAMS build            COMPONENT scripts DESTINATION scripts)
install(PROGRAMS rebuild          COMPONENT scripts DESTINATION scripts)
install(PROGRAMS configure.pl     COMPONENT scripts DESTINATION scripts)
install(PROGRAMS mktoplevel.pl    COMPONENT scripts DESTINATION scripts)

file(GLOB dot_cmake *.cmake)
install(FILES ${dot_cmake}        COMPONENT scripts DESTINATION scripts)

install(DIRECTORY docs            COMPONENT scripts DESTINATION scripts/docs)
install(FILES Doxyfile.in         COMPONENT scripts DESTINATION scripts)
