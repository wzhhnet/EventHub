###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.4)

project(evthub
  LANGUAGES C CXX
  VERSION "1.0.0"
)

option(TEST "Build with gtest" OFF)
option(GEN_SHARED_LIB "Build shared library" OFF)

if (TEST)
add_definitions(-DTEST_ON)
endif ()

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(C_TARGET evthub)
set(CPP_TARGET eventhub)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc	
)

add_subdirectory(c)
add_subdirectory(c++)
if (TEST)
add_subdirectory(test)
endif ()
