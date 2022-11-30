###############################################################################
#    Model Element   : CMakeLists
#    Component       : EventHub
#    File Name       : CMakeLists.txt
#    Author          : wanch
###############################################################################
cmake_minimum_required(VERSION 3.10)

include_directories(
	${PROJECT_SOURCE_DIR}/c
	${PROJECT_SOURCE_DIR}/c++
)

set(GTEST_TARGET ${PROJECT_NAME}_test)
set(SAMPLE_TARGET EventHubSample)

file(GLOB GTEST_SRC event_hub_test.cpp)
file(GLOB SAMPLE_SRC EventHubSample.cpp)

add_executable(${GTEST_TARGET} ${GTEST_SRC})
target_link_libraries(${GTEST_TARGET} LINK_PUBLIC gtest)

add_executable(${SAMPLE_TARGET} ${SAMPLE_SRC})
target_link_libraries(${SAMPLE_TARGET} LINK_PUBLIC ${CPP_TARGET})


