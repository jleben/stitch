

#add_executable(try test.cpp)
#target_link_libraries(try concurrency pthread)

add_library(testing STATIC ../testing/testing.cpp)

function(make_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} testing concurrency pthread)
endfunction()

make_test(test-file test_file.cpp)

make_test(test-timer test_timer.cpp)

make_test(test-signal test_signal.cpp)

#make_test(test-waiter waiter_test.cpp)

make_test(test-spsc-queue test_spsc_queue.cpp)

make_test(test-mpmc-journal-queue test_mpmc_journal_queue.cpp)

make_test(test-mpsc-journal-queue test_mpsc_journal_queue.cpp)

make_test(test-streams test_streams.cpp)

# FIXME: The common stream is now wait-free, so a special RT stream is not needed
# We need to port over the batch push/pop methods though.
make_test(test-rt-stream rt_stream_test.cpp)
