add_library(shadedpath
  #ThreadResources.cpp
  ShadedPathEngine.cpp
)
target_include_directories(shadedpath
  PRIVATE public_headers
  INTERFACE public_headers)