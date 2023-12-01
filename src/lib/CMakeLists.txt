add_library(shadedpath
  #ThreadResources.cpp
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