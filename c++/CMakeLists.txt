###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.10)

aux_source_directory(. SRC_LIBS)

if (GEN_SHARED_LIB)
	add_library(${CPP_TARGET} SHARED ${SRC_LIBS})
else ()
	add_library(${CPP_TARGET} STATIC ${SRC_LIBS})
endif ()

