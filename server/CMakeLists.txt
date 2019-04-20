set(SERVER pichi)

add_executable(${SERVER} main.cpp run.cpp)
target_include_directories(${SERVER} PRIVATE ${CMAKE_BINARY_DIR}/server)
target_link_libraries(${SERVER} PRIVATE
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
)

install(TARGETS ${SERVER} DESTINATION bin)
