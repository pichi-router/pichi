find_path(MaxmindDB_INCLUDE_DIR NAMES maxminddb.h)
find_library(MaxmindDB_LIBRARY NAMES maxminddb)

if (MaxmindDB_INCLUDE_DIR)
  file(STRINGS "${MaxmindDB_INCLUDE_DIR}/maxminddb.h" version_line
        REGEX "^#define[\t ]+PACKAGE_VERSION[\t ]+\".*\"")
  if (version_line)
    string(REGEX REPLACE "^#define[\t ]+PACKAGE_VERSION[\t ]+\"(.*)\""
            "\\1" MaxmindDB_VERSION_STRING "${version_line}")
    unset(version_line)
  endif (version_line)
endif (MaxmindDB_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MaxmindDB
                                  REQUIRED_VARS MaxmindDB_INCLUDE_DIR MaxmindDB_LIBRARY
                                  VERSION_VAR MaxmindDB_VERSION_STRING)
set(MaxmindDB_FOUND ${MaxmindDB_FOUND})

if (MaxmindDB_FOUND)
  set(MaxmindDB_INCLUDE_DIRS ${MaxmindDB_INCLUDE_DIR})
  set(MaxmindDB_LIBRARIES ${MaxmindDB_LIBRARY})
endif (MaxmindDB_FOUND)
