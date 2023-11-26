# Preamble
cmake_minimum_required(VERSION 3.27)
project(ShadedPathV VERSION 0.1.0 LANGUAGES C CXX)
enable_testing()
# cmake folder
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# Project wide setup
set(BUILD_SHARED_LIBS NO)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
# Externally provided content
include(cmake/dependencies.cmake)
# Main targets built by this project
add_subdirectory(src/lib)
add_subdirectory(src/app)

# Not typically needed if there is a parent project
if(PROJECT_IS_TOP_LEVEL)
#[=[
  add_subdirectory(tests)
  add_subdirectory(packaging)
#]=]
endif()

