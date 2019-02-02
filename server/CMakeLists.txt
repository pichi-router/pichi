set(SERVER pichi)

include(CheckIncludeFiles)
include(CheckFunctionExists)
check_include_files("unistd.h" HAS_UNISTD_H)
check_include_files("pwd.h" HAS_PWD_H)
check_include_files("grp.h" HAS_GRP_H)
check_function_exists("getpwnam" HAS_GETPWNAM)
check_function_exists("setuid" HAS_SETUID)
check_function_exists("getgrnam" HAS_GETGRNAM)
check_function_exists("setgid" HAS_SETGID)
check_function_exists("fork" HAS_FORK)
check_function_exists("setsid" HAS_SETSID)
check_function_exists("close" HAS_CLOSE)

configure_file(config.h.in ${CMAKE_BINARY_DIR}/server/config.h)

add_executable(${SERVER} main.cpp)
target_include_directories(${SERVER} PRIVATE ${CMAKE_BINARY_DIR}/server)
target_link_libraries(${SERVER} PRIVATE
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
)

install(TARGETS ${SERVER} DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/test/geo.mmdb DESTINATION share/pichi)
