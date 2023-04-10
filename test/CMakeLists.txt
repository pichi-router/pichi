list(APPEND RAW_TESTS keys hash hmac hkdf base64 method cryptogram router uri endpoint
  socks5 http ss balancer trojan)
list(APPEND VO_TESTS vos vo_credential vo_ingress vo_egress vo_rule vo_route vo_options)

configure_file(geo.mmdb ${CMAKE_CURRENT_BINARY_DIR}/geo.mmdb COPYONLY)

set(UTILS_SRC utils.cpp)
set(VO_SRC vo.cpp)

foreach(CASE IN LISTS RAW_TESTS)
  add_executable(${CASE} "${CASE}.cpp" ${UTILS_SRC})
endforeach()

foreach(CASE IN LISTS VO_TESTS)
  add_executable(${CASE} "${CASE}.cpp" ${UTILS_SRC} ${VO_SRC})
endforeach()

foreach(CASE IN LISTS RAW_TESTS VO_TESTS)
  add_test(NAME ${CASE} COMMAND ${CASE})
  target_link_libraries(${CASE} PRIVATE ${PICHI_LIBRARY}
    ${COMMON_LIBRARIES} Boost::unit_test_framework)
  set_target_properties(${CASE} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_CRT})
endforeach()

include(GetLibraryDirectories)

foreach(lib IN LISTS COMMON_LIBRARIES)
  _get_library_directories(dir ${lib})
  list(APPEND COMMON_LIB_DIRS ${dir})
endforeach()

list(REMOVE_DUPLICATES COMMON_LIB_DIRS)

if(ENABLE_CONAN AND BUILD_SHARED_LIBS)
  if(WIN32)
    foreach(LIB_DIR IN LISTS COMMON_LIB_DIRS)
      list(APPEND TEST_PATH "$<PATH:APPEND,${LIB_DIR},..,${CMAKE_INSTALL_BINDIR}>")
    endforeach()

    set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES ENVIRONMENT
      "PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>\;$<JOIN:${TEST_PATH},\;>")
  elseif(APPLE)
    set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES ENVIRONMENT
      "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>:$<JOIN:${COMMON_LIB_DIRS},:>")
  else()
    set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES ENVIRONMENT
      "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>:$<JOIN:${COMMON_LIB_DIRS},:>")
  endif()
endif()
