###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.4)

aux_source_directory(. SRC_LIBS)

if (GEN_SHARED_LIB)
	add_library(${C_TARGET} SHARED ${SRC_LIBS})
else ()
	add_library(${C_TARGET} STATIC ${SRC_LIBS})
endif ()

target_link_libraries(${C_TARGET} LINK_PUBLIC pthread)

