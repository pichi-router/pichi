message(STATUS "Generating config.h")

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)

message(STATUS "Generating config.h - done")
