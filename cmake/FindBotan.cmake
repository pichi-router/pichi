find_path(Botan_INCLUDE_DIRS NAMES botan/version.h PATH_SUFFIXES botan-3)
find_library(Botan_LIBRARIES NAMES botan-3)

if(Botan_INCLUDE_DIRS AND Botan_LIBRARIES)
  if(NOT CMAKE_CROSSCOMPILING)
    try_run(VERSION_FOUND _ ${CMAKE_BINARY_DIR}/botan ${CMAKE_SOURCE_DIR}/cmake/test/botan-version.cpp
      CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${Botan_INCLUDE_DIRS}
      LINK_LIBRARIES ${Botan_LIBRARIES}
      RUN_OUTPUT_VARIABLE Botan_VERSION
      CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  else()
    set(Botan_VERSION "3.5.0")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Botan
  REQUIRED_VARS Botan_LIBRARIES Botan_INCLUDE_DIRS
  VERSION_VAR Botan_VERSION
)

if(Botan_FOUND)
  include(AddFoundTarget)
  _add_found_target(botan::botan "${Botan_INCLUDE_DIRS}" "${Botan_LIBRARIES}")
endif()
