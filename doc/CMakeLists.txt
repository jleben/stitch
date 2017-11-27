
find_program(SPHINX_BUILD_EXECUTABLE NAMES sphinx-build)

if (NOT SPHINX_BUILD_EXECUTABLE)
    message(FATAL_ERROR "Sphinx build executable not found. Can not build documentation.")
endif()

add_custom_target(docs
    COMMAND ${SPHINX_BUILD_EXECUTABLE} -b html ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
