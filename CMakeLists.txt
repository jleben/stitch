cmake_minimum_required(VERSION 3.27)

project(stitch LANGUAGES CXX)

option(STITCH_BUILD_DOCUMENTATION "Build Stitch documentation." OFF)
option(STITCH_BUILD_TESTS "Build Stitch tests." OFF)
option(STITCH_STATIC_LIB "Build Stitch as a static library." OFF)

include(CheckCompilerFlag)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Werror=return-type)

check_compiler_flag(CXX -mcx16 cxx_flag_mcx16_available)
if(${cxx_flag_mcx16_available})
    message(STATUS "Using compiler flag -mcx16")
    add_compile_options(-mcx16)
else()
    message(STATUS "Compiler flag -mcx16 not available.")
endif()

set(sources
    stitch/hazard_pointers.cpp
    stitch/linux/events.cpp
    stitch/linux/signal.cpp
    stitch/linux/timer.cpp
    stitch/linux/file_event.cpp
    stitch/linux/file.cpp
)

if(STITCH_STATIC_LIB)
    message(STATUS "Stitch building static")
    add_library(stitch STATIC ${sources})
else()
    message(STATUS "Stitch building shared")
    add_library(stitch SHARED ${sources})
endif()

set_property(TARGET stitch PROPERTY CXX_STANDARD 23)

if (STITCH_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()

if (STITCH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
