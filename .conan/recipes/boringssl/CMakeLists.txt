cmake_minimum_required(VERSION 3.8)
project(cmake_wrapper)

include(conanbuildinfo.cmake)
conan_basic_setup(KEEP_RPATH)

if(MSVC)
  add_compile_options(/wd5264)
endif()

add_subdirectory("source_subfolder")
