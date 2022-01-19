cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(pichi)

# Options
option(BUILD_SERVER "Build pichi application" ON)
option(BUILD_TEST "Build unit test cases" ON)
option(STATIC_LINK "Static linking" ON)
option(INSTALL_DEVEL "Install files for development" OFF)
option(ENABLE_CONAN "Enable conan" OFF)

# C++ standard options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preprocessing
if (ENABLE_CONAN)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(KEEP_RPATHS)
  if (CMAKE_CROSSCOMPILING)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  endif ()
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(HandleDependencies)
include(Configure)

# Setup global variables
#   PICHI_LIBRARY:    the target name of pichi library
#   ALL_INCLUDE_DIRS: the including directories for all targets
#   COMMON_LIBRARIES: the libraries for all targets
#   COMMON_LIB_DIRS:  the directories for ${COMMON_LIBRARIES}
#   EXTRA_LIBRARIES:  the libraries for the executable targets
#   INSTALL_RPATH:    the runtime paths for all targets
#   MSVC_CRT:         the correct library name for target property MSVC_RUNTIME_LIBRARY

set(PICHI_LIBRARY pichi_lib)

list(APPEND ALL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
  ${Boost_INCLUDE_DIRS} ${MbedTLS_INCLUDE_DIRS} ${Sodium_INCLUDE_DIRS}
  ${MaxmindDB_INCLUDE_DIRS} ${Rapidjson_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

list(APPEND COMMON_LIBRARIES
  ${Boost_CONTEXT_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}
  ${MbedTLS_LIBRARIES} ${Sodium_LIBRARIES} ${MaxmindDB_LIBRARIES} ${OPENSSL_LIBRARIES})

foreach(LIB IN LISTS COMMON_LIBRARIES)
  get_filename_component(DIR ${LIB} DIRECTORY)
  list(APPEND COMMON_LIB_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES COMMON_LIB_DIRS)

list(APPEND EXTRA_LIBRARIES ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
if (WIN32 AND STATIC_LINK)
  list(APPEND EXTRA_LIBRARIES crypt32)
endif ()

set(INSTALL_RPATH "$<IF:$<BOOL:${APPLE}>,@executable_path/../lib,$ORIGIN/../lib>")
set(MSVC_CRT "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${STATIC_LINK}>>:DLL>")

# Setup the targets
add_subdirectory(src)

if (BUILD_SERVER)
  add_subdirectory(server)
endif ()

if (BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()
