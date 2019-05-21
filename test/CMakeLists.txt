# target_link_libraries(DistributedHashMapTest ${CMAKE_BINARY_DIR}/libbasket.so)

# add_dependencies(DistributedHashMapTest basket)

set(examples DistributedHashMapTest DistributedMapTest DistributedMessageQueueTest DistributedPriorityQueueTest dq_test)
# How do people test structures like these?
foreach (example ${examples})
    add_executable (${example} ${example}.cpp)
    add_dependencies(${example} basket)
    target_include_directories(${example} PRIVATE "${CMAKE_BINARY_DIR}/")
    set_target_properties(${example} PROPERTIES ENV TEST_ENV="LD_PRELOAD=-L${CMAKE_BINARY_DIR}/libbasket.so")
    target_link_libraries(${example} ${LIB_FLAGS} -L${CMAKE_BINARY_DIR}/ -lbasket)
    set_target_properties (${example} PROPERTIES FOLDER test)
    set (test_parameters "")
    add_test(NAME ${example} COMMAND "$<TARGET_FILE:${example}>" ${test_parameters})
    set_tests_properties(${example} PROPERTIES ENV TEST_ENV="LD_PRELOAD=${CMAKE_BINARY_DIR}/libbasket.so")
    set_tests_properties(${example} PROPERTIES WILL_FAIL true)
    target_link_libraries(${example} ${CMAKE_BINARY_DIR}/libbasket.so)
endforeach()
