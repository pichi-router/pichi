find_path(MbedTLS_INCLUDE_DIRS NAMES mbedtls/version.h)

find_library(MbedTLS_LIBRARY NAMES mbedtls libmbedtls)
find_library(MbedTLS_CRYPTO_LIBRARY NAMES mbedcrypto libmbedcrypto)

if (MbedTLS_INCLUDE_DIRS)
  file(STRINGS "${MbedTLS_INCLUDE_DIRS}/mbedtls/version.h" version_line
        REGEX "^#define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\".*\"")
  if (version_line)
    string(REGEX REPLACE "^#define[\t ]+MBEDTLS_VERSION_STRING[\t ]+\"(.*)\""
            "\\1" MbedTLS_VERSION_STRING "${version_line}")
    unset(version_line)
  endif ()
endif ()

set(MbedTLS_LIBRARIES ${MbedTLS_LIBRARY} ${MbedTLS_CRYPTO_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS
  REQUIRED_VARS MbedTLS_LIBRARY MbedTLS_CRYPTO_LIBRARY MbedTLS_INCLUDE_DIRS MbedTLS_VERSION_STRING
  VERSION_VAR MbedTLS_VERSION_STRING
)
