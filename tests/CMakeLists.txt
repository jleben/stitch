

#add_executable(try test.cpp)
#target_link_libraries(try concurrency pthread)

add_library(testing STATIC ../testing/testing.cpp)

function(make_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} testing concurrency pthread)
endfunction()

make_test(test-file file_test.cpp)

make_test(test-timer timer_test.cpp)

make_test(test-signal signal_test.cpp)

#make_test(test-waiter waiter_test.cpp)

make_test(test-rt-stream rt_stream_test.cpp)

make_test(test-stream stream_test.cpp)

make_test(test-mpmc-journal-queue mpmc_journal_queue_test.cpp)

make_test(test-stream-gen test_stream_gen.cpp)
