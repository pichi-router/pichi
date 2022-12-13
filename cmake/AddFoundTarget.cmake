macro(_add_found_target target inc lib)
  if(NOT TARGET ${target})
    add_library(${target} UNKNOWN IMPORTED)
    set_target_properties(${target} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${inc}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${lib}")
  endif()
endmacro()
