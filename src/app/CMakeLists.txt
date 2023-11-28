add_executable(app
  LandscapeGenerator.cpp
  ShadedPathV.cpp
)
target_link_libraries(app PRIVATE shadedpath)