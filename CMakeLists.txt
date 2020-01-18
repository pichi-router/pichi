cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(pichi LANGUAGES CXX)

option(BUILD_SERVER "Build pichi application" ON)
option(BUILD_TEST "Build unit test cases" ON)
option(STATIC_LINK "Static linking" ON)
option(INSTALL_DEVEL "Install files for development" OFF)
option(ENABLE_TLS "Enable TLS adapters" ON)

set(PICHI_LIBRARY pichi_lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(ProcessOptions)

find_package(Boost 1.67.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)
find_package(Threads REQUIRED)
if (ENABLE_TLS)
  find_package(OpenSSL REQUIRED)
  include_directories(${OPENSSL_INCLUDE_DIR})
  link_libraries(${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS})
endif ()

include(Configure)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/include
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
endif ()

if (BUILD_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()
