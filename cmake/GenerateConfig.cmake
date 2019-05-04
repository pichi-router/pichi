message(STATUS "Generating config.h")

if (Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")
  # From libsodium 1.0.17, the declarations of crypto_stream_xxx functions contain
  #   '__attribute__ ((nonnull))', which might let GCC cause '-Wignored-attributes' warning
  #   if using std::is_same to detect function signature equation.
  set(NO_IGNORED_ATTRIBUTES_FOR_SODIUM ON)
endif (Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)

message(STATUS "Generating config.h - done")
