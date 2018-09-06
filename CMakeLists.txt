cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
project(pichi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(MbedTLS REQUIRED)
find_package(Sodium REQUIRED)
find_package(MaxmindDB REQUIRED)
find_package(Rapidjson REQUIRED)
find_package(Threads REQUIRED)

add_definitions("-DBOOST_COROUTINES_V2 -DBOOST_ASIO_NO_DEPRECATED")

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${MbedTLS_INCLUDE_DIRS}
  ${Sodium_INCLUDE_DIRS}
  ${MaxmindDB_INCLUDE_DIRS}
  ${Rapidjson_INCLUDE_DIRS}
)

link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_subdirectory(src)

link_libraries(
  ${MbedTLS_LIBRARIES}
  ${Sodium_LIBRARIES}
  ${MaxmindDB_LIBRARY} 
  ${PICHI_LIBRARY}
)

add_subdirectory(server)

enable_testing()
add_subdirectory(test)
