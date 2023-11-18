# build from root folder:
# cmake -B ./build -S ./ 
# cmake --build ./build
# clean, after that build will rebuild all:
# cmake --build ./build --target clean

# macOS req:
# brew install glfw
# install *darwin* libktx package from khronos
cmake_minimum_required(VERSION 3.26)
project(
  Hello
  VERSION 0.0.1
  LANGUAGES CXX)
#add_executable(Hello)
include_directories(../libraries/vulkan/macOS/include)
include_directories(../libraries/glfw/include)
include_directories(../libraries/ktx/lib/ktx.framework/Headers)
include_directories(../libraries)
find_package(glfw3 3.3 REQUIRED)
message(STATUS "Path to package glfw = ${glfw}")
find_package(ktx 4.3 REQUIRED)
message(STATUS "Path to package ktx = KTX::ktx")
find_library(libktx NAMES libktx.dylib PATHS /usr/local/lib REQUIRED)
message(STATUS "Path to library ktx = ${libktx}")
#KTX::ktx SHARED
#find_library(glfw NAME libglfw3.dylib PATHS ../libraries/glfw/lib-arm64 REQUIRED)
#message(STATUS "Path to library glfw = ${glfw}")
find_library(vulkan1 NAMES libvulkan.1.dylib PATHS ../libraries/vulkan/macOS/lib REQUIRED)
message(STATUS "Path to vulkan lib = ${vulkan1}")
find_library(vulkan13 NAMES libvulkan.1.3.250.dylib PATHS ../libraries/vulkan/macOS/lib REQUIRED)
message(STATUS "Path to vulkan lib = ${vulkan13}")
add_subdirectory(ShadedPathV)