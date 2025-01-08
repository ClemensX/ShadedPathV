# Preamble
cmake_minimum_required(VERSION 3.27)
project(ShadedPathV VERSION 0.1.0 LANGUAGES C CXX)
enable_testing()
# local options excluded in git, useful entries below
# should not be necessary for regular vulkan device,
# but for small graphics like Intel Iris Xe
# add_definitions(-DUSE_SMALL_GRAPHICS)
# add_definitions(-DOVERRIDE_UNIFORM_BUFFER_DESCRIPTOR_COUNT)
# add_definitions(-DALLOW_USING_NON_TRANSFER_ONLY_QUEUE)
#add_definitions(-DUSE_FIXED_PHYSICAL_DEVICE_INDEX)
add_definitions(-DPHYSICAL_DEVICE_INDEX=0)
include(cmake/local_options.cmake)
if(MSVC)
  message(STATUS "UNICODE chars enabled for MSVC")
  add_definitions(-DUNICODE -D_UNICODE)
  add_compile_options(/sdl)
  # comment next line to get deprecation errors:
  add_compile_options(/wd4996)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated")
elseif(APPLE)
  add_definitions(-DALLOW_USING_NON_TRANSFER_ONLY_QUEUE)
  add_compile_options(-Wall -Wextra -Werror -Wno-error=unused-variable -Wno-error=unused-function
  -Wno-error=implicit-function-declaration -Wno-return-type -Wno-unused-parameter -Wno-unknown-pragmas
  -Wno-defaulted-function-deleted -Wno-unused-private-field -Wno-sign-compare -Wno-deprecated-declarations
  -Wno-unused-but-set-variable -Wno-format-security -Wno-null-conversion -Wno-error=deprecated
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
# comment next line to disable building the tests
# if included, my_test will be built - run test cases in VS 2022 TestExplorer or on command line by executing my_test from it's folder
add_subdirectory(src/test)

# Not typically needed if there is a parent project
if(PROJECT_IS_TOP_LEVEL)
#[=[
  add_subdirectory(tests)
  add_subdirectory(packaging)
#]=]
endif()

# call chain:
# cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=~/dev/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=C:\tools\vcpkg\scripts\buildsystems\vcpkg.cmake
# cmake --build ./build