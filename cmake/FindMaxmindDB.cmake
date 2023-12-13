find_path(MaxmindDB_INCLUDE_DIRS NAMES maxminddb.h PATH_SUFFIXES maxminddb)
find_library(MaxmindDB_LIBRARY NAMES maxminddb)
set(MaxmindDB_LIBRARIES ${MaxmindDB_LIBRARY})

if(MaxmindDB_INCLUDE_DIRS)
  file(STRINGS "${MaxmindDB_INCLUDE_DIRS}/maxminddb.h" version_line
    REGEX "^#define[\t ]+PACKAGE_VERSION[\t ]+\".*\"")

  if(version_line)
    string(REGEX REPLACE "^#define[\t ]+PACKAGE_VERSION[\t ]+\"(.*)\""
      "\\1" MaxmindDB_VERSION_STRING "${version_line}")
    unset(version_line)
  else()
    try_compile(HAS_MMDB_LIB_VERSION
      ${CMAKE_BINARY_DIR}/maxminddb ${CMAKE_SOURCE_DIR}/cmake/test/maxminddb-version.cpp
      CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${MaxmindDB_INCLUDE_DIRS}
      LINK_LIBRARIES ${MaxmindDB_LIBRARY}
    )

    if(HAS_MMDB_LIB_VERSION AND NOT CMAKE_CROSSCOMPILING)
      try_run(VERSION_FOUND _
        ${CMAKE_BINARY_DIR}/maxminddb ${CMAKE_SOURCE_DIR}/cmake/test/maxminddb-version.cpp
        CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${MaxmindDB_INCLUDE_DIRS}
        LINK_LIBRARIES ${MaxmindDB_LIBRARY}
        RUN_OUTPUT_VARIABLE MaxmindDB_VERSION_STRING
        CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    elseif(HAS_MMDB_LIB_VERSION)
      set(MaxmindDB_VERSION_STRING "1.8.0")
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MaxmindDB
  REQUIRED_VARS MaxmindDB_LIBRARY MaxmindDB_LIBRARIES
  MaxmindDB_INCLUDE_DIRS MaxmindDB_VERSION_STRING
  VERSION_VAR MaxmindDB_VERSION_STRING
)

if(MaxmindDB_FOUND)
  include(AddFoundTarget)
  _add_found_target(maxminddb::maxminddb "${MaxmindDB_INCLUDE_DIRS}" "${MaxmindDB_LIBRARY}")
endif()
