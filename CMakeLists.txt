# Preamble
cmake_minimum_required(VERSION 3.27)
project(ShadedPathV VERSION 0.1.0 LANGUAGES C CXX)
enable_testing()
if(MSVC)
  message(STATUS "UNICODE chars enabled for MSVC")
  add_definitions(-DUNICODE -D_UNICODE)
elseif(APPLE)
  #message(STATUS "AAAAAA")
  add_compile_options(-Wall -Wextra -Werror -Wno-error=unused-variable -Wno-error=unused-function
  -Wno-error=implicit-function-declaration -Wno-return-type -Wno-unused-parameter -Wno-unknown-pragmas
  -Wno-defaulted-function-deleted -Wno-unused-private-field -Wno-sign-compare -Wno-deprecated-declarations
  -Wno-unused-but-set-variable -Wno-format-security -Wno-null-conversion
  )
endif()
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
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_subdirectory(src/lib)
add_subdirectory(src/app)

# Not typically needed if there is a parent project
if(PROJECT_IS_TOP_LEVEL)
#[=[
  add_subdirectory(tests)
  add_subdirectory(packaging)
#]=]
endif()

# call chain:
# cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=~/dev/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=c:\dev\vulkan\vcpkg\scripts\buildsystems\vcpkg.cmake
# cmake --build ./build