# Things that we expect will have to be built from source
include(FetchContent)
#FetchContent_Declare(privateThing ...)
#FetchContent_Declare(patchedDep ...)
#FetchContent_MakeAvailable(privateThing patchedDep)
# Other things the developer is responsible for making
# available to us by whatever method they prefer
#find_package(fmt REQUIRED)
#find_package(zlib REQUIRED
message(STATUS vcpkg: ${VCPKG_ROOT})
message(STATUS "ShadedPathV engine dev setup")
message(STATUS "libraries needed preinstalled: vulkan")
message(STATUS "checking libraries:")
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 CONFIG REQUIRED)
find_path(GLFW_INC GLFW REQUIRED)
message(STATUS "glfw path = ${GLFW_INC}")
find_package(ktx 4.3 CONFIG REQUIRED)
find_path(KTX_INC ktx.h REQUIRED)
message(STATUS "ktx headers = ${KTX_INC}")
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# check glsl compiler is found:
if(NOT Vulkan_glslc_FOUND)
  message(ERROR "Vulkan glslc not found")
endif()


message(STATUS "Vulkan libraries = ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan headers = ${Vulkan_INCLUDE_DIRS}")
#message(STATUS "glfw libraries = ${glfw3_LIBRARIES}")
#message(STATUS "glfw libraries = ${glfw}")
#message(STATUS "glfw he = ${glfw3_INCLUDE_DIRS}")
#message(STATUS "glfw he = ${glfw_INCLUDE_DIRS}")
#find_path(KTX_INC NAMES ktxvulkan.h)
#message(STATUS "ktx headers = ${KTX_INC}")
#find_path(GLM_INC NAMES vec4.hpp)
find_path(GLM_INC GLM REQUIRED)
message(STATUS "glm headers = ${GLM_INC}")

#disable OpenXR if include not found:
#message(STATUS "cur path  ${CMAKE_SOURCE_DIR}")
#message(STATUS "openxr path  ${CMAKE_SOURCE_DIR}/../OpenXR-SDK/include/openxr")
#message(STATUS "openxr lib  ${CMAKE_SOURCE_DIR}/../OpenXR-SDK/build/win64/src/loader/Debug")
#link_directories(${CMAKE_SOURCE_DIR}/libs/openxr)
find_path(OPENXR_INC PATHS ${CMAKE_SOURCE_DIR}/../OpenXR-SDK/include/openxr NAMES openxr.h)
if (OPENXR_INC)
  add_definitions(-DOPENXR_AVAILABLE)
  include_directories(${OPENXR_INC}/../)
  link_directories(${OPENXR_INC}/../../build/win64/src/loader/Debug)
  link_directories(${OPENXR_INC}/../../build/win64/src/loader/Release)
else()
  message(WARNING "OpenXR build disabled")
endif()
message(STATUS "OpenXR headers = ${OPENXR_INC}")


include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${GLFW_INC})
include_directories(${KTX_INC})
include_directories(${GLM_INC})

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
