###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.10)

if (GEN_SHARED_LIB)
	add_library(${CPP_TARGET} SHARED EventHub.cpp)
else ()
	add_library(${CPP_TARGET} STATIC EventHub.cpp)
endif ()

