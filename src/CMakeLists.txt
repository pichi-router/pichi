file(GLOB_RECURSE SRCS *.cpp)
if (STATIC_LINK)
  add_library(${PICHI_LIBRARY} STATIC ${SRCS})
  install(TARGETS ${PICHI_LIBRARY} ARCHIVE DESTINATION lib)
else (STATIC_LINK)
  add_library(${PICHI_LIBRARY} SHARED ${SRCS})
  target_link_libraries(${PICHI_LIBRARY} PRIVATE
    ${Boost_CONTEXT_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
  )
  if (WIN32)
    target_link_libraries(${PICHI_LIBRARY} PRIVATE ${Boost_DATE_TIME_LIBRARY})
    set_target_properties(${PICHI_LIBRARY} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif (WIN32)
  if (VERSION)
    set_target_properties(${PICHI_LIBRARY} PROPERTIES VERSION ${VERSION})
  endif(VERSION)
  install(TARGETS ${PICHI_LIBRARY} LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
endif (STATIC_LINK)
set_target_properties(${PICHI_LIBRARY} PROPERTIES OUTPUT_NAME pichi)

if (INSTALL_HEADERS)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/pichi DESTINATION include)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/boost DESTINATION include)
endif (INSTALL_HEADERS)

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/src/config.h)
target_include_directories(${PICHI_LIBRARY} PRIVATE ${CMAKE_BINARY_DIR}/src)
