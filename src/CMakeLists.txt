file(GLOB_RECURSE SRCS *.cpp)
if (STATIC_LINK)
  add_library(${PICHI_LIBRARY} STATIC ${SRCS})
else (STATIC_LINK)
  add_library(${PICHI_LIBRARY} SHARED ${SRCS})
  target_link_libraries(${PICHI_LIBRARY} PRIVATE
    ${Boost_CONTEXT_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
  )
endif (STATIC_LINK)
set_target_properties(${PICHI_LIBRARY} PROPERTIES OUTPUT_NAME pichi)
