cmake_minimum_required(VERSION 3.10)

project(basket VERSION 1.0.0 DESCRIPTION "Basket: A library for flexible distributed data containers")

include(GNUInstallDirs)
include_directories(src/basket)
include_directories(src)
include_directories(include)
set(BASKET_SRC  include/basket/common/singleton.h
                include/basket/common/debug.h
                src/basket/common/debug.cpp
                include/basket/common/constants.h
                include/basket/common/typedefs.h
                include/basket/common/data_structures.h
                src/basket/common/data_structures.cpp
                include/basket/communication/rpc_lib.h
                src/basket/communication/rpc_lib.cpp
                include/basket/hashmap/distributed_hash_map.h
                src/basket/hashmap/distributed_hash_map.cpp
                include/basket/map/distributed_map.h
        src/basket/map/distributed_map.cpp
        include/basket/multimap/distributed_multi_map.h
        src/basket/multimap/distributed_multi_map.cpp
        include/basket/clock/global_clock.h
        src/basket/clock/global_clock.cpp
        include/basket/queue/distributed_message_queue.h
        src/basket/queue/distributed_message_queue.cpp
        include/basket/priority_queue/distributed_priority_queue.h
        src/basket/priority_queue/distributed_priority_queue.cpp
        include/basket/sequencer/global_sequence.h
        src/basket/sequencer/global_sequence.cpp)

add_library(${PROJECT_NAME} SHARED ${BASKET_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/basket.h)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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

option(BUILD_TEST "Build the unit tests" FALSE)
if(BUILD_TEST)
    enable_testing()
    include(CTest)
    add_subdirectory(test)
    foreach (example ${examples})
        add_custom_command(TARGET ${example} POST_BUILD
                                  COMMAND ${CMAKE_COMMAND} -E copy
                                  "" $<TARGET_FILE_DIR:${example}>)
    endforeach()
endif()
