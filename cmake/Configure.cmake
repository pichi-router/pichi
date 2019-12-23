# C/C++ Macros
add_compile_definitions(BOOST_ASIO_NO_DEPRECATED)

# Find out whether class template argument deduction is supported for std::array
message(STATUS "Checking P0702R1 feature")
try_compile(HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  ${CMAKE_BINARY_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/test/P0702R1.cpp)
if (HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
  message(STATUS "Checking P0702R1 feature - yes")
else (HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
  message(STATUS "Checking P0702R1 feature - no")
endif (HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)

# Enable complaining all warnings as errors
if (MSVC)
  add_compile_options(/W4 /WX)
else (MSVC)
  add_compile_options(-Wall -Wextra -pedantic -Werror)
endif (MSVC)

# Options for Microsoft C++
if (MSVC)
  # Linking with the correct universal CRT library
  # https://blogs.msdn.microsoft.com/vcblog/2015/03/03/introducing-the-universal-crt/
  set(CRT_FLAG "/M")
  if (STATIC_LINK)
    set(CRT_FLAG "${CRT_FLAG}T")
  else (STATIC_LINK)
    set(CRT_FLAG "${CRT_FLAG}D")
  endif (STATIC_LINK)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CRT_FLAG "${CRT_FLAG}d")
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(${CRT_FLAG})

  # In MSVC, the default Exception Handling option is /EHsc, Which wouldn't catch exceptions
  #   thrown by boost::context. '/EHs' would make it correctly at least.
  # Further information:
  # - https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2017
  # - https://www.boost.org/doc/libs/release/libs/context/doc/html/context/requirements.html
  add_compile_options(/EHs)

  # Avoid C1128 error
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(/bigobj)
  endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

  # Avoid warning STL4015, caused by rapidjson
  add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

  # Avoid warning STL4009, which should be incorrectly complained.
  #  https://github.com/chriskohlhoff/asio/issues/290
  if (MSVC_VERSION VERSION_LESS "1920")
    add_compile_definitions(_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
  endif (MSVC_VERSION VERSION_LESS "1920")
endif (MSVC)

if (WIN32 AND CMAKE_SYSTEM_VERSION)
  string(REGEX REPLACE "^([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" major ${CMAKE_SYSTEM_VERSION})
  string(REGEX REPLACE "^[0-9]+\.([0-9]+)\.[0-9]+$" "\\1" minor ${CMAKE_SYSTEM_VERSION})
  math(EXPR win32_version "(${major} << 8) + ${minor}" OUTPUT_FORMAT HEXADECIMAL)
  add_compile_definitions(_WIN32_WINNT=${win32_version})
endif (WIN32 AND CMAKE_SYSTEM_VERSION)

# Generating config.hpp
message(STATUS "Generating config.hpp")

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

# From Boost-Beast 1.70.0, MSVC generates warning C4702(unreachable code) when including
#   boost/beast/http/fields.hpp. Plz refer to https://github.com/boostorg/beast/issues/1582 .
# Disable this warning if 1.70.0 or later version are going to be used.
if (Boost_VERSION_STRING VERSION_GREATER_EQUAL "1.70.0")
  set(DISABLE_C4702_FOR_BEAST_FIELDS ON)
else (Boost_VERSION_STRING VERSION_GREATER_EQUAL "1.70.0")
  set(DISABLE_C4702_FOR_BEAST_FIELDS OFF)
endif (Boost_VERSION_STRING VERSION_GREATER_EQUAL "1.70.0")

if (Boost_VERSION_STRING VERSION_LESS "1.70.0")
  set(RESOLVER_CONSTRUCTED_FROM_EXECUTOR OFF)
else (Boost_VERSION_STRING VERSION_LESS "1.70.0")
  # From version 1.70.0, Boost.Asio changed the behaviour of resolver::resolver,
  #   that it's supposed to be constructed from Executor instead of ExecutionContext.
  set(RESOLVER_CONSTRUCTED_FROM_EXECUTOR ON)
endif (Boost_VERSION_STRING VERSION_LESS "1.70.0")

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
endif (BUILD_SERVER)

configure_file(${CMAKE_SOURCE_DIR}/include/pichi/config.hpp.in ${CMAKE_BINARY_DIR}/include/pichi/config.hpp)

message(STATUS "Generating config.hpp - done")
