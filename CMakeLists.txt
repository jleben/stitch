cmake_minimum_required(VERSION 3.1)

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

set_property(TARGET concurrency PROPERTY CXX_STANDARD 14)

add_subdirectory(tests)
