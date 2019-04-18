cmake_minimum_required(VERSION 3.10)

project(basket VERSION 1.0.0 DESCRIPTION "Basket: A library for flexible distributed data containers")

include(GNUInstallDirs)

set(BASKET_SRC src/singleton.h src/debug.h src/constants.h src/typedefs.h src/data_structures.h src/communication/rpc_lib.h src/hashmap/DistributedHashMap.h src/map/DistributedMap.h src/multimap/DistributedMultiMap.h src/clock/global_clock.h src/queue/DistributedMessageQueue.h src/priority_queue/DistributedPriorityQueue.h src/sequencer/global_sequence.h)

add_library(${PROJECT_NAME} SHARED ${BASKET_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/basket.h)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

configure_file(basket.pc.in basket.pc @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE .)

set(LIB_FLAGS -lrpc -lmpi -lpthread -lrt -ldl -lboost_filesystem)
target_link_libraries(basket ${LIB_FLAGS})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/basket.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
