set(BoostVersion    1.54.0)
set(BoostComponents system filesystem thread date_time program_options log log_setup regex chrono serialization iostreams)

# set(Boost_DEBUG 1)
set(BOOST_INCLUDEDIR $ENV{HOME}/coraltools/libboost/include)
set(BOOST_LIBRARYDIR $ENV{HOME}/coraltools/libboost/lib)
set(BOOST_ROOT /opt/ibm/boost)
find_package(Boost ${BoostVersion} REQUIRED COMPONENTS ${BoostComponents})

message("boost includes: ${Boost_INCLUDE_DIRS}")
message("boost librarys: ${Boost_LIBRARY_DIRS}")

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

#
# Boost install steps:
#
# http://downloads.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.gz
# tar -xvzf /bghome/tgooding/boost_1_59_0.tar.gz 
# cd boost_1_59_0/
# ./bootstrap.sh --prefix=/opt/boost
# ./b2 -j8 --layout=tagged threading=multi link=shared address-model=64 install
