find_path(Sodium_INCLUDE_DIR NAMES sodium/version.h)

find_library(Sodium_LIBRARY NAMES sodium)

if (Sodium_INCLUDE_DIR)
  file(STRINGS "${Sodium_INCLUDE_DIR}/sodium/version.h" version_line
        REGEX "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\".*\"")
  if (version_line)
    string(REGEX REPLACE "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\"(.*)\""
            "\\1" Sodium_VERSION_STRING "${version_line}")
    unset(version_line)
  endif (version_line)
endif (Sodium_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sodium
                                  REQUIRED_VARS Sodium_INCLUDE_DIR Sodium_LIBRARY
                                  VERSION_VAR Sodium_VERSION_STRING)
set(Sodium_FOUND ${Sodium_FOUND})

if (Sodium_FOUND)
  set(Sodium_INCLUDE_DIRS ${Sodium_INCLUDE_DIR})
  set(Sodium_LIBRARIES ${Sodium_LIBRARY})
endif (Sodium_FOUND)
