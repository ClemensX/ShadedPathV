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

add_executable(app
   AppSupport.cpp
   DeviceCoordApp.cpp
   GeneratedTexturesApp.cpp
   gltfTerrainApp.cpp
   gltfObjectsApp.cpp
   BillboardDemo.cpp
   LandscapeDemo1.cpp
#   LandscapeGenerator.cpp
   LineApp.cpp
   SimpleApp.cpp
  Incoming.cpp
  TextureViewer.cpp
  SimpleMultiApp.cpp
  SimpleMultiWin.cpp
  ShadedPathV.cpp
  ${VULKAN_SHADERS}
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

target_link_libraries(app PRIVATE shadedpath)
target_precompile_headers(app REUSE_FROM shadedpath)

# shader compile take from openxr tutorial:
source_group("Shaders" FILES ${VULKAN_SHADERS})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shader.bin)

function(compile_glsl run_target_name)
    set(glsl_output_files "")
    foreach(in_file IN LISTS ARGN)
        #get_filename_component(glsl_stage ${in_file} NAME_WE)
        get_filename_component(glsl_stage ${in_file} NAME)
        set(out_file ${CMAKE_CURRENT_BINARY_DIR}/shader.bin/${glsl_stage}.spv)
        #set(out_file ${CMAKE_CURRENT_BINARY_DIR}/${in_file}.spv)
            # Run glslc if we can find it
            message(STATUS "command: Vulkan::glslc in: ${in_file} -o out: ${out_file}")
            # -g option is debug info
            # -O option is optimization
            add_custom_command(
                OUTPUT ${out_file}
                COMMAND Vulkan::glslc -O ${in_file} -o ${out_file}
                DEPENDS ${in_file}
            )
        list(APPEND glsl_output_files ${out_file})
    endforeach()
    add_custom_target(${run_target_name} ALL DEPENDS ${glsl_output_files})
    #set_target_properties(${run_target_name} PROPERTIES FOLDER ${HELPER_FOLDER})
    #message(ERROR "nix")
endfunction()

# compile shaders to ./build/src/app/shader.bin/
compile_glsl(glsl_compiles ${VULKAN_SHADERS})
add_dependencies(app
    glsl_compiles
)
