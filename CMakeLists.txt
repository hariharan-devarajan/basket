cmake_minimum_required(VERSION 3.10)

project(hdds VERSION 1.0.0 DESCRIPTION "Hermes Distributed Data Structures")

include(GNUInstallDirs)

set(HDDS_SRC src/singleton.h src/debug.h src/constants.h src/typedefs.h src/data_structures.h src/communication/rpc_lib.h src/hashmap/DistributedHashMap.h src/map/DistributedMap.h src/multimap/DistributedMultiMap.h src/clock/global_clock.h src/queue/DistributedMessageQueue.h src/priority_queue/DistributedPriorityQueue.h)

add_library(${PROJECT_NAME} SHARED ${HDDS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/hdds.h)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

configure_file(hdds.pc.in hdds.pc @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE .)

set(LIB_FLAGS -lrpc -lmpi -lpthread -lrt -ldl -lboost_filesystem)
target_link_libraries(hdds ${LIB_FLAGS})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/hdds.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
