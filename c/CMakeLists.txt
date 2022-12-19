###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.4)

if (GEN_SHARED_LIB)
	add_library(${C_TARGET} SHARED event_hub.c)
else ()
	add_library(${C_TARGET} STATIC event_hub.c)
endif ()

target_link_libraries(${C_TARGET} LINK_PUBLIC pthread)

