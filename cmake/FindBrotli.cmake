find_path(Brotli_INCLUDE_DIRS NAMES brotli/decode.h)
find_library(Brotli_COMMON_LIBRARY NAMES brotlicommon brotlicommon-static)
find_library(Brotli_DECODER_LIBRARY NAMES "brotlidec" "brotlidec-static")
find_library(Brotli_ENCODER_LIBRARY NAMES "brotlienc" "brotlienc-static")

set(Brotli_LIBRARIES ${Brotli_COMMON_LIBRARY} ${Brotli_DECODER_LIBRARY} ${Brotli_ENCODER_LIBRARY})

if(NOT CMAKE_CROSSCOMPILING)
  try_run(VERSION_FOUND _ ${CMAKE_BINARY_DIR}/brotli ${CMAKE_SOURCE_DIR}/cmake/test/brotli-version.cpp
    CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${Brotli_INCLUDE_DIRS}
    COMPILE_DEFINITIONS -DBROTLI_DECODER_VERSION
    LINK_LIBRARIES ${Brotli_LIBRARIES}
    RUN_OUTPUT_VARIABLE Brotli_VERSION
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
else()
  set(Brotli_VERSION "1.0.0")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brotli
  REQUIRED_VARS Brotli_COMMON_LIBRARY Brotli_DECODER_LIBRARY Brotli_ENCODER_LIBRARY
  Brotli_LIBRARIES Brotli_INCLUDE_DIRS
  VERSION_VAR Brotli_VERSION
)

if(Brotli_FOUND)
  include(AddFoundTarget)
  _add_found_target(brotli::common "${Brotli_INCLUDE_DIRS}" "${Brotli_COMMON_LIBRARY}")
  _add_found_target(brotli::decoder "${Brotli_INCLUDE_DIRS}" "${Brotli_DECODER_LIBRARY}")
  target_link_libraries(brotli::decoder INTERFACE brotli::common)
  _add_found_target(brotli::encoder "${Brotli_INCLUDE_DIRS}" "${Brotli_ENCODER_LIBRARY}")
  target_link_libraries(brotli::encoder INTERFACE brotli::common)
  add_library(brotli::brotli INTERFACE IMPORTED)
  target_link_libraries(brotli::brotli INTERFACE brotli::decoder brotli::encoder)
endif()
