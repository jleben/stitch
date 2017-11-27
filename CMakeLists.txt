cmake_minimum_required(VERSION 3.1)

project(stitch)

option(STITCH_BUILD_DOCUMENTATION OFF)

set(sources
    stitch/hazard_pointer.cpp
    stitch/linux/events.cpp
    stitch/linux/signal.cpp
    stitch/linux/timer.cpp
    stitch/linux/file_event.cpp
    stitch/linux/file.cpp
)

add_library(stitch SHARED ${sources})

set_property(TARGET stitch PROPERTY CXX_STANDARD 14)

add_subdirectory(tests)

if (STITCH_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()
