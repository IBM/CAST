set(USE_CLANG OFF CACHE BOOL "build application with clang")   # OFF is the default
set(USE_CLANGANALYZE OFF CACHE BOOL "run build with clang analyzer")   # OFF is the default

message("Option USE_CLANG is ${USE_CLANG}")
if(${USE_CLANG})
    SET (CMAKE_C_COMPILER              "/usr/local/bin/clang")
    SET (CMAKE_CXX_COMPILER            "/usr/local/bin/clang++")
    SET (CMAKE_AR                      "/usr/local/bin/llvm-ar")
    SET (CMAKE_LINKER                  "/usr/local/bin/llvm-ld")
    SET (CMAKE_NM                      "/usr/local/bin/llvm-nm")
    SET (CMAKE_OBJDUMP                 "/usr/local/bin/llvm-objdump")
    SET (CMAKE_RANLIB                  "/usr/local/bin/llvm-ranlib")

# Boost 1.60 has a known problem with defined() expansion triggered by clang 3.9 adding -Wexpansion-to-defined to -Wall
# Current solution is to suppress warnings via -Wno-expansion-to-defined.  See Boost ticket:
#     https://svn.boost.org/trac/boost/ticket/12144
#
    SET (CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} -Wno-expansion-to-defined")

#    SET (CMAKE_C_FLAGS                "-Wall")
#    SET (CMAKE_C_FLAGS_DEBUG          "-g")
#    SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#    SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
#    SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
#    SET (CMAKE_CXX_FLAGS                "-Wall")
#    SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
#    SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
#    SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
#    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
endif(${USE_CLANG})
