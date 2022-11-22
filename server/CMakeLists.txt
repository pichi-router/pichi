set(SERVER pichi)

add_executable(${SERVER} main.cpp run.cpp)

target_link_libraries(${SERVER} PRIVATE ${PICHI_LIBRARY}
  ${COMMON_LIBRARIES} Boost::program_options Boost::filesystem)
set_target_properties(${SERVER} PROPERTIES
  INSTALL_RPATH ${INSTALL_RPATH}
  MSVC_RUNTIME_LIBRARY ${MSVC_CRT}
  INSTALL_RPATH_USE_LINK_PATH TRUE)
install(TARGETS ${SERVER} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
