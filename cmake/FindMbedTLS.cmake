find_path(MbedTLS_INCLUDE_DIR NAMES mbedtls/version.h)

find_library(MbedTLS_LIBRARY NAMES mbedtls)
find_library(MbedTLS_CRYPTO_LIBRARY NAMES mbedcrypto)

if (MbedTLS_INCLUDE_DIR)
  file(STRINGS "${MbedTLS_INCLUDE_DIR}/mbedtls/version.h" version_line
        REGEX "^#define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\".*\"")
  if (version_line)
    string(REGEX REPLACE "^#define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\"(.*)\""
            "\\1" MbedTLS_VERSION_STRING "${version_line}")
    unset(version_line)
  endif (version_line)
endif (MbedTLS_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS
                                  REQUIRED_VARS MbedTLS_INCLUDE_DIR MbedTLS_LIBRARY MbedTLS_CRYPTO_LIBRARY
                                  VERSION_VAR MbedTLS_VERSION_STRING)
set(MbedTLS_FOUND ${MBEDTLS_FOUND})

if (MbedTLS_FOUND)
  set(MbedTLS_INCLUDE_DIRS ${MbedTLS_INCLUDE_DIR})
  set(MbedTLS_LIBRARIES ${MbedTLS_LIBRARY} ${MbedTLS_CRYPTO_LIBRARY})
endif (MbedTLS_FOUND)
