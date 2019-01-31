cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(pichi LANGUAGES CXX)

option(BUILD_SERVER "Build pichi application" ON)
option(BUILD_TEST "Build unit test cases" ON)
option(STATIC_LINK "Static linking" ON)
option(INSTALL_HEADERS "Install header files" OFF)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(PICHI_LIBRARY pichi_lib)

if (STATIC_LINK)
  if (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  else (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
  endif (UNIX)
endif (STATIC_LINK)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(BOOST_COMPONENTS context system)
if (BUILD_SERVER)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} program_options filesystem)
endif (BUILD_SERVER)
if (BUILD_TEST)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif (BUILD_TEST)

find_package(Boost 1.67.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)
find_package(Threads REQUIRED)

add_definitions("-DBOOST_COROUTINES_V2 -DBOOST_ASIO_NO_DEPRECATED")

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
  ${MbedTLS_INCLUDE_DIRS}
  ${Sodium_INCLUDE_DIRS}
  ${MaxmindDB_INCLUDE_DIRS}
  ${Rapidjson_INCLUDE_DIRS}
)

link_libraries(
  ${MbedTLS_LIBRARIES}
  ${Sodium_LIBRARIES}
  ${MaxmindDB_LIBRARIES} 
  ${CMAKE_THREAD_LIBS_INIT}
)

add_subdirectory(src)

link_libraries(${PICHI_LIBRARY})

if (BUILD_SERVER)
  add_subdirectory(server)
endif (BUILD_SERVER)

if (BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif (BUILD_TEST)
