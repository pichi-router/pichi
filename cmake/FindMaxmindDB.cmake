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
  _add_found_target(MaxmindDB::maxminddb "${MaxmindDB_INCLUDE_DIRS}" "${MaxmindDB_LIBRARY}")
endif()
