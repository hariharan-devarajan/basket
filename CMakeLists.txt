cmake_minimum_required(VERSION 3.10)

project(basket VERSION 1.0.0 DESCRIPTION "Basket: A library for flexible distributed data containers")

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_RPCLIB=true")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_THALLIUM_TCP=true")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_THALLIUM_ROCE=true")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include(GNUInstallDirs)
include_directories(src/basket)
include_directories(src)
include_directories(include)
set(BASKET_SRC  include/basket/common/singleton.h
                include/basket/common/debug.h
		include/basket/common/enumerations.h
		include/basket/common/macros.h
		include/basket/common/configuration_manager.h
                src/basket/common/debug.cpp
                include/basket/common/constants.h
                include/basket/common/typedefs.h
                include/basket/common/data_structures.h
                src/basket/common/data_structures.cpp
                include/basket/communication/rpc_lib.h
                src/basket/communication/rpc_lib.cpp
                include/basket/unordered_map/unordered_map.h
                include/basket/map/map.h
                include/basket/multimap/multimap.h
                include/basket/clock/global_clock.h
                src/basket/clock/global_clock.cpp
                include/basket/queue/queue.h
                include/basket/priority_queue/priority_queue.h
                include/basket/sequencer/global_sequence.h
                src/basket/sequencer/global_sequence.cpp
	)

add_library(${PROJECT_NAME} SHARED ${BASKET_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/basket.h)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

configure_file(basket.pc.in basket.pc @ONLY)

target_include_directories(${PROJECT_NAME} PRIVATE .)

if(DEFINED BASKET_ENABLE_RPCLIB)
    set(RPC_LIB_FLAGS -lrpc ${RPC_LIB_FLAGS})
endif()
if(DEFINED BASKET_ENABLE_THALLIUM_ROCE OR DEFINED BASKET_ENABLE_THALLIUM_TCP)
    set(RPC_LIB_FLAGS -lthallium -lmercury -lmercury_util -lmargo -labt ${RPC_LIB_FLAGS})
endif()
set(LIB_FLAGS ${RPC_LIB_FLAGS} -lmpi -lpthread -lrt -lboost_filesystem)

# libs for NATS tests
#set(LIB_FLAGS ${LIB_FLAGS} -lnats)

# libs for Redis tests
#set(LIB_FLAGS ${LIB_FLAGS} -lev -L/home/kbateman/install/lib/ -L/home/kbateman/install/lib64/ -lhiredis -lredox)

target_link_libraries(basket ${LIB_FLAGS})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_BINARY_DIR}/basket.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

option(BUILD_TEST "Build the unit tests" ON)
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
