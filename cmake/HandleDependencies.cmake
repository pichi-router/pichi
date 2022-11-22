# To find boost
list(APPEND BOOST_COMPONENTS context system thread)

if(BUILD_SERVER)
  list(APPEND BOOST_COMPONENTS filesystem program_options)
endif()

if(BUILD_TEST)
  list(APPEND BOOST_COMPONENTS unit_test_framework)
endif()

set(Boost_NO_BOOST_CMAKE ON)
set(Boost_NO_SYSTEM_PATHS ${ENABLE_CONAN})

if(BUILD_SHARED_LIBS)
  set(Boost_USE_STATIC_LIBS OFF)
else()
  set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost 1.77.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED)

# To find other necessary dependencies
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

# To find OpenSSL/BoringSSL
if(TLS_FINGERPRINT)
  find_package(BoringSSL 12 REQUIRED)
  find_package(Brotli REQUIRED)
else()
  find_package(OpenSSL REQUIRED)
endif()

find_package(Threads REQUIRED)

# Setup COMMON_LIBRARIES for later usage
list(APPEND COMMON_LIBRARIES
  OpenSSL::SSL Boost::boost Boost::context Boost::system
  MbedTLS::tls Sodium::sodium MaxmindDB::maxmind Rapidjson::rapidjson
  Threads::Threads ${CMAKE_DL_LIBS})

if(Brotli_FOUND)
  list(APPEND COMMON_LIBRARIES Brotli::decoder Brotli::encoder)
endif()
