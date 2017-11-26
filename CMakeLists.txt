cmake_minimum_required(VERSION 3.1)

project(stitch)

set(sources
    concurrency/hazard_pointer.cpp
    concurrency/linux/events.cpp
    concurrency/linux/signal.cpp
    concurrency/linux/timer.cpp
    concurrency/linux/file_event.cpp
    concurrency/linux/file.cpp
)

add_library(stitch SHARED ${sources})

set_property(TARGET stitch PROPERTY CXX_STANDARD 14)

add_subdirectory(tests)
