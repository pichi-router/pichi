# C/C++ Macros
add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)
if (NOT STATIC_LINK)
  add_compile_definitions(BOOST_ALL_DYN_LINK)
  add_compile_definitions(SODIUM_STATIC)
endif ()

# Find out whether class template argument deduction is supported for std::array
message(STATUS "Checking P0702R1 feature")
try_compile(HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  ${CMAKE_BINARY_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/test/P0702R1.cpp)
if (HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
  message(STATUS "Checking P0702R1 feature - yes")
else ()
  message(STATUS "Checking P0702R1 feature - no")
endif ()

# Enable complaining all warnings as errors
if (MSVC)
  add_compile_options(/W4 /WX)
else ()
  add_compile_options(-Wall -Wextra -Werror)
endif ()

# Options for Microsoft C++
if (MSVC)
  # In MSVC, the default Exception Handling option is /EHsc, Which wouldn't catch exceptions
  #   thrown by boost::context. '/EHs' would make it correctly at least.
  # Further information:
  # - https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2017
  # - https://www.boost.org/doc/libs/release/libs/context/doc/html/context/requirements.html
  add_compile_options(/EHs)

  # Avoid C1128 error
  add_compile_options(/bigobj)

  # Avoid warning STL4015, caused by rapidjson
  add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

  # Avoid warning STL4009, which should be incorrectly complained.
  #  https://github.com/chriskohlhoff/asio/issues/290
  if (MSVC_VERSION VERSION_LESS "1920")
    add_compile_definitions(_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
  endif ()
endif ()

if (WIN32 AND CMAKE_SYSTEM_VERSION)
  string(REGEX REPLACE "^([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" major ${CMAKE_SYSTEM_VERSION})
  string(REGEX REPLACE "^[0-9]+\.([0-9]+)\.[0-9]+$" "\\1" minor ${CMAKE_SYSTEM_VERSION})
  math(EXPR win32_version "(${major} << 8) + ${minor}" OUTPUT_FORMAT HEXADECIMAL)
  add_compile_definitions(_WIN32_WINNT=${win32_version})
endif ()

# Generating config.hpp
message(STATUS "Generating config.hpp")

if (Sodium_VERSION_STRING VERSION_GREATER_EQUAL "1.0.17")
  # From libsodium 1.0.17, the declarations of crypto_stream_xxx functions contain
  #   '__attribute__ ((nonnull))', which might let GCC cause '-Wignored-attributes' warning
  #   if using std::is_same to detect function signature equation.
  set(DISABLE_GCC_IGNORED_ATTRIBUTES ON)
endif ()

if (Boost_VERSION_STRING VERSION_LESS "1.74.0")
  set(DISABLE_CLANG_C11_EXTENTIONS)
endif ()

if (BUILD_SERVER)
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
endif ()

configure_file(${CMAKE_SOURCE_DIR}/include/pichi/common/config.hpp.in
  ${CMAKE_BINARY_DIR}/include/pichi/common/config.hpp)

message(STATUS "Generating config.hpp - done")
