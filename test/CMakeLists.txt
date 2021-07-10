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
  target_include_directories(${CASE} PRIVATE ${ALL_INCLUDE_DIRS})
  target_link_libraries(${CASE} PRIVATE ${PICHI_LIBRARY}
    ${COMMON_LIBRARIES} ${EXTRA_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  set_target_properties(${CASE} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_CRT})
endforeach()

list(JOIN COMMON_LIB_DIRS ":" UNIX_RUN_PATH_ENV)
list(JOIN COMMON_LIB_DIRS ";" WIN32_RUN_PATH_ENV)
if (WIN32)
  set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES
    ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>;${WIN32_RUN_PATH_ENV}")
elseif (APPLE)
  set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES
    ENVIRONMENT "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>:${UNIX_RUN_PATH_ENV}")
else ()
  set_tests_properties(${RAW_TESTS} ${VO_TESTS} PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:${PICHI_LIBRARY}>:${UNIX_RUN_PATH_ENV}")
endif ()
