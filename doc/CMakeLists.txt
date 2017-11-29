
find_program(DOXYGEN_EXECUTABLE NAMES doxygen)

if (NOT DOXYGEN_EXECUTABLE)
    message(FATAL_ERROR "Doxygen executable not found. Can not build documentation.")
endif()

configure_file(doxy-input doxy-input)

add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doxy-config
)
