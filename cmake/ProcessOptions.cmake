# Force to use static linking for iOS
if (IOS)
  if (NOT STATIC_LINK)
    message(WARNING "Force to use static linking on iOS")
    set(STATIC_LINK ON)
  endif (NOT STATIC_LINK)
  if (BUILD_SERVER OR BUILD_TEST)
    message(WARNING "Executable file is prohibited on iOS")
    set(BUILD_SERVER OFF)
    set(BUILD_TEST OFF)
  endif (BUILD_SERVER OR BUILD_TEST)
endif (IOS)

# Setting library suffix for linking
if (STATIC_LINK)
  if (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  else (UNIX)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
  endif (UNIX)
endif (STATIC_LINK)

# Using rpath for dynamic linking
if (UNIX AND NOT STATIC_LINK)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif (UNIX AND NOT STATIC_LINK)

set(BOOST_COMPONENTS context system)
if (WIN32 AND NOT STATIC_LINK)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} date_time)
endif (WIN32 AND NOT STATIC_LINK)
if (BUILD_SERVER)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem program_options)
endif (BUILD_SERVER)
if (BUILD_TEST)
  set(BOOST_COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)
endif (BUILD_TEST)

# C++ standard options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
