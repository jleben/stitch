

add_executable(try test.cpp)
target_link_libraries(try concurrency pthread)

add_executable(test-file test_file.cpp)
target_link_libraries(test-file concurrency pthread)
