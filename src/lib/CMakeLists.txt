add_library(shadedpath
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw.cpp
  imgui/imgui_impl_vulkan.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui.cpp
  ThreadResources.cpp
  ShadedPathEngine.cpp
  Files.cpp
  GameTime.cpp
  Texture.cpp
  Object.cpp
  Sound.cpp
)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

#message(STATUS "current includes")
#GET_DIRECTORY_PROPERTY(output INCLUDE_DIRECTORIES)
#MESSAGE(WARNING ${output})
#message(STATUS "current includes enmd")

target_include_directories(shadedpath
  PRIVATE public_headers
  INTERFACE public_headers)