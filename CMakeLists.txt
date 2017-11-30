cmake_minimum_required(VERSION 3.1)

project(stitch)

option(STITCH_BUILD_DOCUMENTATION OFF)
option(STITCH_BUILD_TESTS OFF)
option(STITCH_STATIC_LIB OFF)

set(sources
    stitch/hazard_pointers.cpp
    stitch/linux/events.cpp
    stitch/linux/signal.cpp
    stitch/linux/timer.cpp
    stitch/linux/file_event.cpp
    stitch/linux/file.cpp
)

if(STITCH_STATIC_LIB)
    add_library(stitch STATIC ${sources})
else()
    add_library(stitch SHARED ${sources})
endif()

set_property(TARGET stitch PROPERTY CXX_STANDARD 14)

if (STITCH_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()

if (STITCH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
