cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(pichi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Boost 1.67.0 REQUIRED
  COMPONENTS context system program_options unit_test_framework REQUIRED
)
find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.0.0 REQUIRED)
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

add_subdirectory(server)

enable_testing()
add_subdirectory(test)
