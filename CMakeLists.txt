cmake_minimum_required(VERSION 3.0)

project(concurrency)

set(sources
    linux/events.cpp
    #linux/waiter.cpp
    linux/signal.cpp
    linux/timer.cpp
    linux/file_event.cpp
    linux/file.cpp
)

add_library(concurrency SHARED ${sources})

add_subdirectory(tests)
