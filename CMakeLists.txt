cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(pichi)

option(BUILD_SERVER "Build pichi application" ON)
option(BUILD_TEST "Build unit test cases" ON)
option(STATIC_LINK "Static linking" ON)
option(INSTALL_DEVEL "Install files for development" OFF)
option(ENABLE_CONAN "Enable conan" ON)

if (ENABLE_CONAN)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup(KEEP_RPATHS)
endif ()

set(PICHI_LIBRARY pichi_lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(ProcessOptions)

find_package(Boost 1.72.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include(Configure)

list(APPEND ALL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
  ${Boost_INCLUDE_DIRS} ${MbedTLS_INCLUDE_DIRS} ${Sodium_INCLUDE_DIRS}
  ${MaxmindDB_INCLUDE_DIRS} ${Rapidjson_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

list(APPEND COMMON_LIBRARIES
  ${Boost_CONTEXT_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${MbedTLS_LIBRARIES}
  ${Sodium_LIBRARIES} ${MaxmindDB_LIBRARIES} ${OPENSSL_LIBRARIES})
list(APPEND EXTRA_LIBRARIES ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
if (WIN32 AND STATIC_LINK)
  list(APPEND EXTRA_LIBRARIES crypt32)
endif ()

foreach(LIB IN LISTS COMMON_LIBRARIES)
  get_filename_component(DIR ${LIB} DIRECTORY)
  list(APPEND COMMON_LIB_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES COMMON_LIB_DIRS)

add_subdirectory(src)

if (BUILD_SERVER)
  add_subdirectory(server)
endif ()

if (BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()
