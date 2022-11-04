list(APPEND BOOST_COMPONENTS context system thread)

if(WIN32 AND BUILD_SHARED_LIBS)
  list(APPEND BOOST_COMPONENTS date_time)
endif()

if(BUILD_SERVER)
  list(APPEND BOOST_COMPONENTS filesystem program_options)
endif()

if(BUILD_TEST)
  list(APPEND BOOST_COMPONENTS unit_test_framework)
endif()

find_package(Threads REQUIRED)

set(Boost_NO_SYSTEM_PATHS ${ENABLE_CONAN})

if(BUILD_SHARED_LIBS)
  set(Boost_USE_STATIC_LIBS OFF)
else()
  set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost 1.77.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)

if(UNIX)
  if(NOT BUILD_SHARED_LIBS)
    list(PREPEND CMAKE_FIND_LIBRARY_SUFFIXES .a)
  elseif(APPLE)
    list(PREPEND CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
  else()
    list(PREPEND CMAKE_FIND_LIBRARY_SUFFIXES .so)
  endif()
endif()

list(REMOVE_DUPLICATES CMAKE_FIND_LIBRARY_SUFFIXES)

if(ENABLE_CONAN)
  set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
else()
  set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH ON)
endif()

find_package(MbedTLS 2.7.0 REQUIRED)
find_package(Sodium 1.0.12 REQUIRED)
find_package(MaxmindDB 1.3.0 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)

if(TLS_FINGERPRINT)
  find_package(BoringSSL 12 REQUIRED)
  find_package(Brotli REQUIRED)
else()
  find_package(OpenSSL REQUIRED)
endif()

# Setup global variables
# ALL_INCLUDE_DIRS: the including directories for all targets
# COMMON_LIBRARIES: the libraries for all targets
# COMMON_LIB_DIRS:  the directories for ${COMMON_LIBRARIES}
# EXTRA_LIBRARIES:  the libraries for the executable targets
list(APPEND ALL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
  ${Boost_INCLUDE_DIRS} ${MbedTLS_INCLUDE_DIRS} ${Sodium_INCLUDE_DIRS}
  ${MaxmindDB_INCLUDE_DIRS} ${Rapidjson_INCLUDE_DIRS} ${Brotli_INCLUDE_DIRS})

list(APPEND COMMON_LIBRARIES
  ${Boost_CONTEXT_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY}
  ${MbedTLS_LIBRARIES} ${Sodium_LIBRARIES} ${MaxmindDB_LIBRARIES} ${OPENSSL_LIBRARIES}
  ${Brotli_LIBRARIES})

foreach(LIB IN LISTS COMMON_LIBRARIES)
  get_filename_component(DIR ${LIB} DIRECTORY)
  list(APPEND COMMON_LIB_DIRS ${DIR})
endforeach()

list(REMOVE_DUPLICATES COMMON_LIB_DIRS)

list(APPEND EXTRA_LIBRARIES ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(WIN32 AND NOT BUILD_SHARED_LIBS)
  list(APPEND EXTRA_LIBRARIES crypt32 bcrypt)
endif()
