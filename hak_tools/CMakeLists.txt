cmake_minimum_required(VERSION 3.8)
project(mpionesided)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -DDEBUG1")
SET(CMAKE_CXX_COMPILER /usr/local/bin/mpic++)
# Require MPI for this project:
find_package(MPI REQUIRED)
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
include_directories(${MPI_INCLUDE_PATH})
set(SOURCE_FILES test/util/city.h test/util/city.cc src/map/DistributedHashMap.h test/distributed_hashmap_test.cpp test/util/debug.h include/distributed_hashmap.h src/common/types.h src/communication/MessagingService.cpp src/communication/MessagingService.h)
add_executable(mpionesided ${SOURCE_FILES})
target_link_libraries(mpionesided ${MPI_LIBRARIES})