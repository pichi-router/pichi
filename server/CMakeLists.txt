set(SERVER pichi)

add_executable(${SERVER} main.cpp)
target_link_libraries(${SERVER} PRIVATE
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
)

install(TARGETS ${SERVER} DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/test/geo.mmdb DESTINATION share/pichi)
