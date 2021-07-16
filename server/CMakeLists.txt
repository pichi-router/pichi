set(SERVER pichi)

add_executable(${SERVER} main.cpp run.cpp)
target_include_directories(${SERVER} PRIVATE ${ALL_INCLUDE_DIRS})
target_link_libraries(${SERVER} PRIVATE ${PICHI_LIBRARY}
  ${COMMON_LIBRARIES} ${EXTRA_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
install(TARGETS ${SERVER} DESTINATION bin)
