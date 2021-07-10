if (ENABLE_CONAN)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(KEEP_RPATHS)
else ()
  # C++ standard options
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "tvOS")
    set(IOS ON)
  else ()
    set(IOS OFF)
  endif ()

  # CMake bug, please refer to https://gitlab.kitware.com/cmake/cmake/issues/16695
  if (IOS)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_USE_PTHREADS_INIT "YES")
  endif ()
endif ()
