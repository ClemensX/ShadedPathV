#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
add_executable(app
  LandscapeGenerator.cpp
  ShadedPathV.cpp
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()
target_link_libraries(app PRIVATE shadedpath)
add_custom_command(TARGET app POST_BUILD
#COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/../lib/*.spv" $<TARGET_FILE_DIR:app>
#COMMAND ${CMAKE_COMMAND} -E copy C:/dev/cpp/cmaketest/build/src/lib/line_vert.spv $<TARGET_FILE_DIR:app>
COMMAND Vulkan::glslc ${PROJECT_SOURCE_DIR}/src/lib/shader/line.vert -o $<TARGET_FILE_DIR:app>/line_vert.spv
)

#add_custom_command(
#  OUTPUT line_vert.spv
#  COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/shader/line.vert -o line_vert.spv
#  #DEPENDS ./shader/line.vert
#)
