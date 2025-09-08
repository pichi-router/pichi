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

include(CMakeDependentOption)
cmake_dependent_option(
  ENABLE_LINK_PATH
  "Whether to enable INSTALL_RPATH_USE_LINK_PATH"
  ON
  "UNIX;ENABLE_CONAN;BUILD_SHARED_LIBS"
  OFF)

# C++ standard options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preprocessing
if(ENABLE_CONAN AND CMAKE_CROSSCOMPILING)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(HandleDependencies)
include(Configure)
include(GNUInstallDirs)

# Setup global variables
# PICHI_LIBRARY:    the target name of pichi library
# RPATH:            the runtime paths for all targets
# MSVC_CRT:         the correct library name for target property MSVC_RUNTIME_LIBRARY
set(PICHI_LIBRARY pichi_lib)
set(RPATH "$<IF:$<BOOL:${APPLE}>,@executable_path/../${CMAKE_INSTALL_LIBDIR},$ORIGIN/../${CMAKE_INSTALL_LIBDIR}>")
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
