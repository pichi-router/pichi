message(STATUS "Generating config.h")

if (Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")
  # From libsodium 1.0.17, the declarations of crypto_stream_xxx functions contain
  #   '__attribute__ ((nonnull))', which might let GCC cause '-Wignored-attributes' warning
  #   if using std::is_same to detect function signature equation.
  set(NO_IGNORED_ATTRIBUTES_FOR_SODIUM ON)
endif (Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")

if (Boost_VERSION_STRING VERSION_LESS "1.69.0")
  # Before BOOST 1.69.0, Clang might complain '-Wreturn-std-move' warning
  #   when dereferencing resolver_results::iterator,
  #   which should be treated as NRVO after C++17.
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wreturn-std-move" HAS_RETURN_STD_MOVE)
  if (HAS_RETURN_STD_MOVE)
    set(NO_RETURN_STD_MOVE_FOR_BOOST_ASIO ON)
  endif (HAS_RETURN_STD_MOVE)
endif (Boost_VERSION_STRING VERSION_LESS "1.69.0")

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)

message(STATUS "Generating config.h - done")
