set (SOURCES
  VulkanResources.cpp
  gltf.cpp
  CubeShader.cpp
  BillboardShader.cpp
  TerrainShader.cpp
  UIShader.cpp
  UI.cpp
  ClearShader.cpp
  SimpleShader.cpp
  pbrShader.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw.cpp
  imgui/imgui_impl_vulkan.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui.cpp
  Util.cpp
  ThreadResources.cpp
  ShadedPathEngine.cpp
  Files.cpp
  GameTime.cpp
  Texture.cpp
  Object.cpp
  Sound.cpp
  LineShader.cpp
  ShaderBase.cpp
  Shaders.cpp
  GlobalRendering.cpp
  GlobalUpdate.cpp
  Presentation.cpp
  ShadedPathEngine.cpp
  World.cpp
  Path.cpp
)
if (OPENXR_INC)
  list(APPEND SOURCES VR.cpp)
endif()
add_library(shadedpath ${SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

#message(STATUS "current includes")
#GET_DIRECTORY_PROPERTY(output INCLUDE_DIRECTORIES)
#MESSAGE(WARNING ${output})
#message(STATUS "current includes enmd")

target_include_directories(shadedpath
  PRIVATE public_headers
  INTERFACE public_headers)
target_precompile_headers(shadedpath PUBLIC [["mainheader.h"]])
target_link_libraries(shadedpath
  PUBLIC
    glfw
    Vulkan::Vulkan
    KTX::ktx
  )
#if (OPENXR_INC)
#target_link_libraries(shadedpath openxr_loaderd)
#endif()
if(APPLE)
  #target_compile_options(shadedpath PUBLIC -Wreturn-type)
endif()
# GLSL C:\dev\vulkan\ShadedPathV\ShadedPathV\ShadedPathVLib\shader\line.vert -o ../x64/Debug/line_vert.spv
# compile shaders:
#message(STATUS "runtime ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
#message(STATUS "runtime ${CMAKE_CURRENT_BINARY_DIR}")
#source_group("shader" FILES ${VULKAN_SHADERS})
#set(glsl_output_files "")
#add_custom_command(
#  OUTPUT line_vert.spv
#  COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/shader/line.vert -o line_vert.spv
#  #DEPENDS ./shader/line.vert
#)
#list(APPEND glsl_output_files line_vert.spv)

#add_custom_target(shader_compiles ALL DEPENDS ${glsl_output_files})
#add_dependencies(shadedpath shader_compiles)
