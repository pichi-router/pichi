# Force to use static linking for iOS
if (IOS)
  if (NOT STATIC_LINK)
    message(WARNING "Force to use static linking on iOS")
    set(STATIC_LINK ON)
  endif (NOT STATIC_LINK)
  if (BUILD_SERVER OR BUILD_TEST)
    message(WARNING "Executable file is prohibited on iOS")
    set(BUILD_SERVER OFF)
    set(BUILD_TEST OFF)
  endif (BUILD_SERVER OR BUILD_TEST)
endif (IOS)

# Setting library suffix for linking
if (STATIC_LINK)
  if (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  else (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
  endif (UNIX)
endif (STATIC_LINK)

# Using rpath for dynamic linking
if (UNIX AND NOT STATIC_LINK)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif (UNIX AND NOT STATIC_LINK)

set(BOOST_COMPONENTS context system)
if (WIN32 AND NOT STATIC_LINK)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} date_time)
endif (WIN32 AND NOT STATIC_LINK)
if (BUILD_SERVER)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem program_options)
endif (BUILD_SERVER)
if (BUILD_TEST)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif (BUILD_TEST)

# C++ standard options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C/C++ Macros
add_definitions(-DBOOST_ASIO_NO_DEPRECATED)

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
endif (MSVC)

# Options for code
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

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)
