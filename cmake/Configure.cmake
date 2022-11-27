if(TRANSPARENT_PF AND APPLE)
  message(STATUS "Downloading the essential Darwin-XNU header files")
  file(DOWNLOAD
    "https://opensource.apple.com/source/xnu/xnu-7195.81.3/libkern/libkern/tree.h"
    "${CMAKE_BINARY_DIR}/include/libkern/tree.h")
  file(DOWNLOAD
    "https://opensource.apple.com/source/xnu/xnu-7195.81.3/bsd/net/pfvar.h"
    "${CMAKE_BINARY_DIR}/include/net/pfvar.h")
  file(DOWNLOAD
    "https://opensource.apple.com/source/xnu/xnu-7195.81.3/bsd/net/radix.h"
    "${CMAKE_BINARY_DIR}/include/net/radix.h")
  message(STATUS "Downloading the essential Darwin-XNU header files -- done")
endif()

# Generating config.hpp
message(STATUS "Generating config.hpp")

if(Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")
  # From libsodium 1.0.17, the declarations of crypto_stream_xxx functions contain
  # '__attribute__ ((nonnull))', which might let GCC cause '-Wignored-attributes' warning
  # if using std::is_same to detect function signature equation.
  set(DISABLE_GCC_IGNORED_ATTRIBUTES ON)
endif()

if(Boost_VERSION_STRING VERSION_LESS "1.74.0")
  set(DISABLE_CLANG_C11_EXTENTIONS)
endif()

if(BUILD_SERVER)
  include(CheckIncludeFiles)
  include(CheckFunctionExists)
  check_include_files("unistd.h" HAS_UNISTD_H)
  check_include_files("signal.h" HAS_SIGNAL_H)
  check_include_files("pwd.h" HAS_PWD_H)
  check_include_files("grp.h" HAS_GRP_H)
  check_function_exists("getpwnam" HAS_GETPWNAM)
  check_function_exists("setuid" HAS_SETUID)
  check_function_exists("getgrnam" HAS_GETGRNAM)
  check_function_exists("setgid" HAS_SETGID)
  check_function_exists("fork" HAS_FORK)
  check_function_exists("setsid" HAS_SETSID)
  check_function_exists("close" HAS_CLOSE)
  check_function_exists("strerror_s" HAS_STRERROR_S)
  check_function_exists("strerror_r" HAS_STRERROR_R)
endif()

configure_file(${CMAKE_SOURCE_DIR}/include/pichi/common/config.hpp.in
  ${CMAKE_BINARY_DIR}/include/pichi/common/config.hpp)

message(STATUS "Generating config.hpp - done")
