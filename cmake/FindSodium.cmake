find_path(Sodium_INCLUDE_DIRS NAMES sodium/version.h)

find_library(Sodium_LIBRARY NAMES sodium libsodium)

set(Sodium_LIBRARIES ${Sodium_LIBRARY})

if(Sodium_INCLUDE_DIRS)
  file(STRINGS "${Sodium_INCLUDE_DIRS}/sodium/version.h" version_line
    REGEX "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\".*\"")

  if(version_line)
    string(REGEX REPLACE "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\"(.*)\""
      "\\1" Sodium_VERSION_STRING "${version_line}")
    unset(version_line)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sodium
  REQUIRED_VARS Sodium_LIBRARY Sodium_LIBRARIES Sodium_INCLUDE_DIRS Sodium_VERSION_STRING
  VERSION_VAR Sodium_VERSION_STRING
)

if(Sodium_FOUND)
  include(AddFoundTarget)
  _add_found_target(Sodium::sodium "${Sodium_INCLUDE_DIRS}" "${Sodium_LIBRARY}")

  # According to libsodium's manual https://libsodium.gitbook.io/doc/usage,
  # SODIUM_STATIC=1 & SODIUM_EXPORT are mandatory when MSVC & static linking.
  if(MSVC)
    message(STATUS "Detecting Sodium libraries type")
    try_compile(Sodium_SHARED
      ${CMAKE_BINARY_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/test/sodium-type.c
      CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${Sodium_INCLUDE_DIRS}
      LINK_LIBRARIES ${Sodium_LIBRARIES}
    )

    if(Sodium_SHARED)
      message(STATUS "Detecting Sodium libraries type - SHARED")
    else()
      set_target_properties(Sodium::sodium PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "SODIUM_STATIC=1;SODIUM_EXPORT")
      message(STATUS "Detecting Sodium libraries type - STATIC")
    endif()
  endif()
endif()
