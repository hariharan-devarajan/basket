cmake_minimum_required(VERSION 3.10)

project(basket VERSION 1.0.0 DESCRIPTION "Basket: A library for flexible distributed data containers")

include(GNUInstallDirs)
include_directories(src/basket)
include_directories(src)
set(BASKET_SRC src/basket/common/singleton.h src/basket/common/debug.h src/basket/common/constants.h src/basket/common/typedefs.h src/basket/common/data_structures.h src/basket/communication/rpc_lib.h src/basket/hashmap/distributed_hash_map.h src/basket/map/distributed_map.h src/basket/multimap/distributed_multi_map.h src/basket/clock/global_clock.h src/basket/queue/distributed_message_queue.h src/basket/priority_queue/distributed_priority_queue.h src/basket/sequencer/global_sequence.h)

add_library(${PROJECT_NAME} SHARED ${BASKET_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/basket.h)
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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
