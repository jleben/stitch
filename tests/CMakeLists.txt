

#add_executable(try test.cpp)
#target_link_libraries(try concurrency pthread)

add_executable(test-file file_test.cpp)
target_link_libraries(test-file concurrency pthread)

add_executable(test-timer timer_test.cpp)
target_link_libraries(test-timer concurrency pthread)

add_executable(test-signal signal_test.cpp)
target_link_libraries(test-signal concurrency pthread)

add_executable(test-waiter waiter_test.cpp)
target_link_libraries(test-waiter concurrency pthread)
