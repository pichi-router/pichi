cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)
project(pichi)

# Options
option(BUILD_SERVER "Build pichi application" ON)
option(BUILD_TEST "Build unit test cases" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(INSTALL_DEVEL "Install files for development" OFF)
option(ENABLE_CONAN "Enable conan" OFF)
option(TRANSPARENT_PF "Implement transparent ingress via Packet Filter" OFF)
option(TRANSPARENT_IPTABLES "Implement transparent ingress via Linux IPTables" OFF)
option(TLS_FINGERPRINT "Enable TLS fingerprint" ON)

# C++ standard options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preprocessing
if(ENABLE_CONAN)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(KEEP_RPATHS)

  if(CMAKE_CROSSCOMPILING)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(HandleDependencies)
include(Configure)

# Setup global variables
# PICHI_LIBRARY:    the target name of pichi library
# INSTALL_RPATH:    the runtime paths for all targets
# MSVC_CRT:         the correct library name for target property MSVC_RUNTIME_LIBRARY
set(PICHI_LIBRARY pichi_lib)
set(INSTALL_RPATH "$<IF:$<BOOL:${APPLE}>,@executable_path/../lib,$ORIGIN/../lib>")
set(MSVC_CRT "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:${BUILD_SHARED_LIBS}>:DLL>")

# Setup the targets
add_subdirectory(src)

if(BUILD_SERVER)
  add_subdirectory(server)
endif()

if(BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
