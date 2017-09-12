cmake_minimum_required(VERSION 3.0)

project(concurrency)

set(sources
    events_linux.cpp
    signal_linux.cpp
    timer_linux.cpp
)

add_library(concurrency SHARED ${sources})

add_subdirectory(tests)
