# build from root folder:
# cmake -B ./build -S ./ 
# cmake --build ./build
# clean, after that build will rebuild all:
# cmake --build ./build --target clean

# cmake --preset=default
# cmake --build build

# macOS req:
# brew install glfw
# install *darwin* libktx package from khronos
cmake_minimum_required(VERSION 3.26)
message(STATUS vcpkg: $ENV{VCPKG_ROOT})
#message(STATUS vcpkg: $ENV{VCPKG_ROOT}\scripts\buildsystems\vcpkg.cmake)
message(STATUS toolchain file: ${CMAKE_TOOLCHAIN_FILE})
project(
  shadedpathv
  VERSION 0.1
  LANGUAGES CXX)

include(${CMAKE_TOOLCHAIN_FILE})
include("help.cmake")

message(STATUS vcpkg: ${VCPKG_ROOT})
message(STATUS "ShadedPathV engine dev setup")
message(STATUS "libraries needed preinstalled: vulkan")
message(STATUS "checking libraries:")
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 CONFIG REQUIRED)
find_package(ktx 4.3 CONFIG REQUIRED)
find_path(GLFW_INC GLFW REQUIRED)
message(STATUS "glfw path = ${GLFW_INC}")


message(STATUS "Vulkan libraries = ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan headers = ${Vulkan_INCLUDE_DIRS}")
message(STATUS "glfw libraries = ${glfw3_LIBRARIES}")
message(STATUS "glfw libraries = ${glfw}")
message(STATUS "glfw he = ${glfw3_INCLUDE_DIRS}")
message(STATUS "glfw he = ${glfw_INCLUDE_DIRS}")

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${GLFW_INC})
#include_directories(${glfw})
#find_library(libktx NAMES libktx.dylib PATHS /usr/local/lib REQUIRED)
#message(STATUS "Path to library ktx = ${libktx}")
#KTX::ktx SHARED
#find_library(glfw NAME libglfw3.dylib PATHS ../libraries/glfw/lib-arm64 REQUIRED)
#message(STATUS "Path to library glfw = ${glfw}")
#find_library(vulkan1 NAMES libvulkan.1.dylib PATHS ../libraries/vulkan/macOS/lib REQUIRED)
#message(STATUS "Path to vulkan lib = ${vulkan1}")
#find_library(vulkan13 NAMES libvulkan.1.3.250.dylib PATHS ../libraries/vulkan/macOS/lib REQUIRED)
#message(STATUS "Path to vulkan lib = ${vulkan13}")
add_subdirectory(ShadedPathV)

