
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/testing/testing.cpp)
    message(FATAL_ERROR "The testing submodule is not present. "
        "Please check out submodules recursively using the following command:\n"
        "git submodule update --init --recursive"
    )
endif()

add_library(testing STATIC ../testing/testing.cpp)
set_property(TARGET testing PROPERTY CXX_STANDARD 14)

function(make_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} testing stitch pthread)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 14)
endfunction()

set(test_sources
    test.cpp
    test_queue_spsc_waitfree.cpp
    test_queue_mpsc_waitfree.cpp
    test_queue_mpmc_waitfree.cpp
    test_queue_mpmc_lockfree.cpp
    test_lockfree_set.cpp
    test_atom_spmc.cpp
    test_streams.cpp
    test_variable.cpp
    test_connections.cpp
    test_signal.cpp
    test_timer.cpp
    test_file.cpp
    test_event_reactor.cpp
)

make_test(tester "${test_sources}")



set(experimental_test_sources
    test_experimental.cpp
)

make_test(test-experimental "${experimental_test_sources}")



#make_test(test-waiter waiter_test.cpp)

# FIXME: The common stream is now wait-free, so a special RT stream is not needed
# We need to port over the batch push/pop methods though.
#make_test(test-rt-stream rt_stream_test.cpp)
