option(USE_MYMATH "Use tutorial provided math implementation" ON)
include(MakeTable.cmake)
add_library(shadedpath.lib mymath.cpp)
target_include_directories(shadedpath.lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shadedpath.lib PUBLIC sp_compiler_flags)
if (USE_MYMATH)
  target_compile_definitions(shadedpath.lib PRIVATE "USE_MYMATH")
  add_library(sqrtLibrary STATIC
      mysqrt.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/Table.h
      )
  target_include_directories(sqrtLibrary PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(sqrtLibrary PUBLIC sp_compiler_flags)
  target_link_libraries(shadedpath.lib PRIVATE sqrtLibrary)
endif()

# install:
set(installable_libs shadedpath.lib sp_compiler_flags)
if(TARGET sqrtLibrary)
  list(APPEND installable_libs sqrtLibrary)
endif()
install(TARGETS ${installable_libs} DESTINATION lib)
install(FILES mymath.h DESTINATION include)

