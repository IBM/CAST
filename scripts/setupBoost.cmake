# Not sure if this is min version or expected version?
# may need to change to 1.66
set(BoostVersion    1.66.0)
set(BoostComponents system filesystem thread date_time program_options log log_setup regex chrono serialization iostreams)

# set(Boost_DEBUG 1)
# Might have to also change these lines? 
#set(BOOST_INCLUDEDIR $ENV{HOME}/coraltools/libboost/include)
#set(BOOST_LIBRARYDIR $ENV{HOME}/coraltools/libboost/lib)
#set(BOOST_ROOT /opt/ibm/boost)
# Changing from the old custom CAST boost to try to point at official boost install dir
set(BOOST_ROOT /usr/local/boost_1_66_0)
set(BOOST_INCLUDEDIR /usr/include)
set(BOOST_LIBRARYDIR /usr/lib64)
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
