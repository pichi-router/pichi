find_path(RapidJSON_INCLUDE_DIRS NAMES rapidjson/rapidjson.h)

if(RapidJSON_INCLUDE_DIRS)
  file(STRINGS "${RapidJSON_INCLUDE_DIRS}/rapidjson/rapidjson.h" major_version_line
    REGEX "^#define[\t ]+RAPIDJSON_MAJOR_VERSION[\t ]+[0-9]*")

  if(major_version_line)
    string(REGEX REPLACE "^#define[\t ]+RAPIDJSON_MAJOR_VERSION[\t ]+([0-9]*)"
      "\\1" major_version "${major_version_line}")
    unset(major_version_line)
  endif()

  file(STRINGS "${RapidJSON_INCLUDE_DIRS}/rapidjson/rapidjson.h" minor_version_line
    REGEX "^#define[\t ]+RAPIDJSON_MINOR_VERSION[\t ]+[0-9]*")

  if(minor_version_line)
    string(REGEX REPLACE "^#define[\t ]+RAPIDJSON_MINOR_VERSION[\t ]+([0-9]*)"
      "\\1" minor_version "${minor_version_line}")
    unset(minor_version_line)
  endif()

  file(STRINGS "${RapidJSON_INCLUDE_DIRS}/rapidjson/rapidjson.h" patch_version_line
    REGEX "^#define[\t ]+RAPIDJSON_PATCH_VERSION[\t ]+[0-9]*")

  if(patch_version_line)
    string(REGEX REPLACE "^#define[\t ]+RAPIDJSON_PATCH_VERSION[\t ]+([0-9]*)"
      "\\1" patch_version "${patch_version_line}")
    unset(patch_version_line)
  endif()

  if(DEFINED major_version AND DEFINED minor_version AND DEFINED patch_version)
    set(RapidJSON_VERSION_STRING "${major_version}.${minor_version}.${patch_version}")
  endif()

  unset(major_version)
  unset(minor_version)
  unset(patch_version)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
  REQUIRED_VARS RapidJSON_INCLUDE_DIRS RapidJSON_VERSION_STRING
  VERSION_VAR RapidJSON_VERSION_STRING
)

if(RapidJSON_FOUND)
  add_library(rapidjson INTERFACE IMPORTED)
  target_include_directories(rapidjson INTERFACE ${RapidJSON_INCLUDE_DIRS})
endif()
