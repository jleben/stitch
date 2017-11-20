
add_library(testing STATIC ../testing/testing.cpp)
set_property(TARGET testing PROPERTY CXX_STANDARD 14)

function(make_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} testing concurrency pthread)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 14)
endfunction()

set(test_sources
    test.cpp
    test_spsc_queue.cpp
    test_mpsc_queue.cpp
    test_mpmc_queue.cpp
    test_lockfree_set.cpp
    test_streams.cpp
    test_signal.cpp
    test_timer.cpp
    test_file.cpp
)

make_test(tester "${test_sources}")

#make_test(test-waiter waiter_test.cpp)

# FIXME: The common stream is now wait-free, so a special RT stream is not needed
# We need to port over the batch push/pop methods though.
#make_test(test-rt-stream rt_stream_test.cpp)
