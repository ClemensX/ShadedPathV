#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
add_executable(app
  LandscapeGenerator.cpp
  ShadedPathV.cpp
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
target_link_libraries(app PRIVATE shadedpath)