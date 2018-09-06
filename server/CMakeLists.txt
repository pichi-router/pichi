set(SERVER pichi)

find_package(Boost 1.66.0 REQUIRED COMPONENTS context system program_options REQUIRED)

add_executable(${SERVER} main.cpp)
target_include_directories(${SERVER} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${SERVER} PRIVATE ${Boost_LIBRARIES})
