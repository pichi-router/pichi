find_path(libsodium_INCLUDE_DIRS NAMES sodium/version.h)

find_library(libsodium_LIBRARY NAMES sodium libsodium)

set(libsodium_LIBRARIES ${libsodium_LIBRARY})

if(libsodium_INCLUDE_DIRS)
  file(STRINGS "${libsodium_INCLUDE_DIRS}/sodium/version.h" version_line
    REGEX "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\".*\"")

  if(version_line)
    string(REGEX REPLACE "^#define[\t ]+SODIUM_VERSION_STRING[\t ]+\"(.*)\""
      "\\1" libsodium_VERSION_STRING "${version_line}")
    unset(version_line)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libsodium
  REQUIRED_VARS libsodium_LIBRARY libsodium_LIBRARIES libsodium_INCLUDE_DIRS libsodium_VERSION_STRING
  VERSION_VAR libsodium_VERSION_STRING
)

if(libsodium_FOUND)
  include(AddFoundTarget)
  _add_found_target(libsodium::libsodium "${libsodium_INCLUDE_DIRS}" "${libsodium_LIBRARY}")

  # According to libsodium's manual https://libsodium.gitbook.io/doc/usage,
  # SODIUM_STATIC=1 & SODIUM_EXPORT are mandatory when MSVC & static linking.
  if(MSVC)
    message(STATUS "Detecting Sodium libraries type")
    try_compile(libsodium_SHARED
      ${CMAKE_BINARY_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/test/sodium-type.c
      CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${libsodium_INCLUDE_DIRS}
      LINK_LIBRARIES ${libsodium_LIBRARIES}
    )

    if(libsodium_SHARED)
      message(STATUS "Detecting Sodium libraries type - SHARED")
    else()
      set_target_properties(Sodium::sodium PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "SODIUM_STATIC=1;SODIUM_EXPORT")
      message(STATUS "Detecting Sodium libraries type - STATIC")
    endif()
  endif()
endif()
