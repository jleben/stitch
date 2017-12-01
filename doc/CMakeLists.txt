
find_program(DOXYGEN_EXECUTABLE NAMES doxygen)

if (NOT DOXYGEN_EXECUTABLE)
    message(FATAL_ERROR "Doxygen executable not found. Can not build documentation.")
endif()

configure_file(doxy-config doxy-config)

add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy-config
)
