file(GLOB_RECURSE SRCS *.cpp)

add_library(${PICHI_LIBRARY} ${SRCS})

# Configure C/C++ macros & compilation options
list(APPEND PICHI_MACROS BOOST_ASIO_NO_DEPRECATED)

if(TRANSPARENT_PF AND APPLE)
  # "-D" prefix is necessary because "PRIVATE" is a key word
  list(APPEND PICHI_MACROS -DPRIVATE)
endif()

if(MSVC)
  # Enable complaining all warnings as errors
  list(APPEND PICHI_OPTIONS /W4 /WX)

  # In MSVC, the default Exception Handling option is /EHsc, Which wouldn't catch exceptions
  # thrown by boost::context. '/EHs' would make it correctly at least.
  # Further information:
  # - https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2017
  # - https://www.boost.org/doc/libs/release/libs/context/doc/html/context/requirements.html
  list(APPEND PICHI_OPTIONS /EHs)

  # Avoid C1128 error
  list(APPEND PICHI_OPTIONS /bigobj)

  # Avoid warning STL4015, caused by rapidjson
  list(APPEND PICHI_MACROS _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

  # Avoid warning STL4009, which should be incorrectly complained.
  # https://github.com/chriskohlhoff/asio/issues/290
  if(MSVC_VERSION VERSION_LESS "1920")
    list(APPEND PICHI_MACROS _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)
  endif()
else()
  # Enable complaining all warnings as errors
  list(APPEND PICHI_OPTIONS -Wall -Wextra -Werror)
endif()

if(WIN32 AND CMAKE_SYSTEM_VERSION)
  string(REGEX REPLACE "^([0-9]+)\.[0-9]+\.[0-9]+$" "\\1" major ${CMAKE_SYSTEM_VERSION})
  string(REGEX REPLACE "^[0-9]+\.([0-9]+)\.[0-9]+$" "\\1" minor ${CMAKE_SYSTEM_VERSION})
  math(EXPR win32_version "(${major} << 8) + ${minor}" OUTPUT_FORMAT HEXADECIMAL)
  list(APPEND PICHI_MACROS _WIN32_WINNT=${win32_version})
endif()

target_compile_options(${PICHI_LIBRARY} PUBLIC ${PICHI_OPTIONS})
target_compile_definitions(${PICHI_LIBRARY} PUBLIC ${PICHI_MACROS})
target_include_directories(${PICHI_LIBRARY} PUBLIC
  ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include)
target_link_libraries(${PICHI_LIBRARY} PRIVATE ${COMMON_LIBRARIES})

set_target_properties(${PICHI_LIBRARY} PROPERTIES
  OUTPUT_NAME pichi
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  MSVC_RUNTIME_LIBRARY ${MSVC_CRT}
  INSTALL_RPATH ${INSTALL_RPATH}
  INSTALL_RPATH_USE_LINK_PATH TRUE)

if(VERSION)
  set_target_properties(${PICHI_LIBRARY} PROPERTIES VERSION ${VERSION})
endif()

if(INSTALL_DEVEL OR BUILD_SHARED_LIBS)
  install(TARGETS ${PICHI_LIBRARY}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(INSTALL_DEVEL)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/pichi DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp")
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/boost DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES ${CMAKE_SOURCE_DIR}/include/pichi.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY ${CMAKE_BINARY_DIR}/include/pichi DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp")
endif()
