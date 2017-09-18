

#add_executable(try test.cpp)
#target_link_libraries(try concurrency pthread)

function(make_test name src)
    add_executable(${name} ${src} utils.cpp)
    target_link_libraries(${name} concurrency pthread)
endfunction()

make_test(test-file file_test.cpp)

make_test(test-timer timer_test.cpp)

make_test(test-signal signal_test.cpp)

make_test(test-waiter waiter_test.cpp)

make_test(test-rt-stream rt_stream_test.cpp)

make_test(test-stream stream_test.cpp)
