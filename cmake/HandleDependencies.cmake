macro(_find_all_dependencies search_mode)
  find_package(Boost 1.77.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS} REQUIRED ${search_mode})
  find_package(MbedTLS 3.0.0 REQUIRED ${search_mode})
  find_package(libsodium 1.0.12 REQUIRED ${search_mode})
  find_package(MaxmindDB 1.3.0 REQUIRED ${search_mode})
  find_package(RapidJSON 1.1.0 REQUIRED EXACT ${search_mode})
  find_package(Threads REQUIRED)

  if(TLS_FINGERPRINT)
    find_package(BoringSSL REQUIRED ${search_mode})
    find_package(Brotli 1.0.0 REQUIRED ${search_mode})
    set(SSL_LIB BoringSSL)
  else()
    find_package(OpenSSL REQUIRED)
    set(SSL_LIB OpenSSL)
  endif()
endmacro()

# To find boost
list(APPEND BOOST_COMPONENTS context system thread)

if(BUILD_SERVER)
  list(APPEND BOOST_COMPONENTS filesystem program_options)
endif()

if(BUILD_TEST)
  list(APPEND BOOST_COMPONENTS unit_test_framework)
endif()

if(ENABLE_CONAN)
  _find_all_dependencies(CONFIG)
else()
  if(BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS OFF)
  else()
    set(Boost_USE_STATIC_LIBS ON)
  endif()

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

  _find_all_dependencies("")
endif()

if(BUILD_TEST)
  # TODO Because Boost_USE_STATIC_LIB=OFF doesn't ensure boost libraries are shared ones,
  # the additional detection is mandatory here to determine
  # whether BOOST_ALL_DYN_LINK is necessary or not.
  message(STATUS "Detecting Boost libraries type")
  try_compile(Boost_TEST_STATIC
    ${CMAKE_BINARY_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/test/boost-test-type.cpp
    CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${Boost_INCLUDE_DIRS}
    COMPILE_DEFINITIONS -DBOOST_ALL_NO_LIB
    LINK_LIBRARIES ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )

  if(Boost_TEST_STATIC)
    message(STATUS "Detecting Boost libraries type - STATIC")
  else()
    set_target_properties(Boost::unit_test_framework PROPERTIES
      INTERFACE_COMPILE_DEFINITIONS "BOOST_UNIT_TEST_FRAMEWORK_DYN_LINK")
    message(STATUS "Detecting Boost libraries type - SHARED")
  endif()
endif()

# Patch OpenSSL::Crypto target when building on windows & dynamic linking
if(WIN32 AND NOT BUILD_SHARED_LIBS)
  get_target_property(deps ${SSL_LIB}::Crypto INTERFACE_LINK_LIBRARIES)

  if(NOT deps)
    unset(deps)
  endif()

  list(APPEND deps crypt32 bcrypt)
  set_target_properties(${SSL_LIB}::Crypto PROPERTIES INTERFACE_LINK_LIBRARIES "${deps}")
endif()

# Setup COMMON_LIBRARIES for later usage
list(APPEND COMMON_LIBRARIES
  Boost::boost Boost::context Boost::system
  MbedTLS::mbedtls libsodium::libsodium MaxmindDB::maxminddb rapidjson
  Threads::Threads ${CMAKE_DL_LIBS} ${SSL_LIB}::SSL)

if(TLS_FINGERPRINT)
  list(APPEND COMMON_LIBRARIES brotli::brotli)
endif()
