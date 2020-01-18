if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "tvOS")
  set(IOS ON)
else ()
  set(IOS OFF)
endif ()

# Setting library suffix for linking
if (STATIC_LINK)
  if (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  else ()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
  endif ()
  set(Boost_USE_STATIC_LIBS ON)
endif ()

# Using rpath for dynamic linking
if (UNIX AND NOT STATIC_LINK)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

set(BOOST_COMPONENTS context system)
if (WIN32 AND NOT STATIC_LINK)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} date_time)
endif ()
if (BUILD_SERVER)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem program_options)
endif ()
if (BUILD_TEST)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif ()

# C++ standard options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake bug, please refer to https://gitlab.kitware.com/cmake/cmake/issues/16695
if (IOS)
  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(CMAKE_USE_PTHREADS_INIT "YES")
endif ()
