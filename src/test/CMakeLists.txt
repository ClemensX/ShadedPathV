set(ShaderFolder ${PROJECT_SOURCE_DIR}/src/lib/shader)
file(GLOB VULKAN_SHADERS
  ${ShaderFolder}/billboard.frag
  ${ShaderFolder}/billboard.geom
  ${ShaderFolder}/billboard.vert
  ${ShaderFolder}/cube.vert
  ${ShaderFolder}/cube.frag
  ${ShaderFolder}/genbrdflut.vert
  ${ShaderFolder}/genbrdflut.frag
  ${ShaderFolder}/line.vert
  ${ShaderFolder}/line.frag
  ${ShaderFolder}/pbr.vert
  ${ShaderFolder}/pbr.frag
  ${ShaderFolder}/triangle.vert
  ${ShaderFolder}/triangle.frag
)

add_executable(my_test
  test.cpp
  ${VULKAN_SHADERS}
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

target_link_libraries(my_test PRIVATE GTest::gtest GTest::gtest_main shadedpath)
target_precompile_headers(my_test REUSE_FROM shadedpath)

# discover google tests and set current directory to app folder:
include(GoogleTest)
gtest_discover_tests(my_test
  DISCOVERY_MODE PRE_TEST
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../app"
    )
