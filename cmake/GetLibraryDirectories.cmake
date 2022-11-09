function(_get_library_directory output lib)
  get_filename_component(dir ${lib} DIRECTORY)
  set(${output} ${dir} PARENT_SCOPE)
endfunction()

function(_get_library_directories output lib)
  if(TARGET ${lib})
    get_target_property(type ${lib} TYPE)

    if("${type}" STREQUAL "STATIC_LIBRARY" OR
      "${type}" STREQUAL "SHARED_LIBRARY" OR
      "${type}" STREQUAL "UNKNOWN_LIBRARY")
      get_target_property(dirs ${lib} INTERFACE_LINK_DIRECTORIES)

      if(NOT dirs)
        unset(dirs)
        get_target_property(deps ${lib} INTERFACE_LINK_LIBRARIES)

        if(deps)
          foreach(dep IN LISTS deps)
            _get_library_directories(dir ${dep})

            if(dir)
              list(APPEND dirs ${dir})
            endif()
          endforeach()
        endif()
      endif()

      get_target_property(lib ${lib} IMPORTED_LOCATION)
      _get_library_directories(dir ${lib})

      if(dirs AND dir)
        list(APPEND dirs ${dir})
      elseif(dir)
        set(dirs ${dir})
      endif()
    endif()
  elseif(EXISTS ${lib})
    _get_library_directory(dirs ${lib})
  endif()

  if(dirs)
    list(REMOVE_DUPLICATES dirs)
    set(${output} ${dirs} PARENT_SCOPE)
  endif()
endfunction()
