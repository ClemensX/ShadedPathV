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
  ${ShaderFolder}/filtercube.vert
  ${ShaderFolder}/irradiancecube.frag
  ${ShaderFolder}/prefilterenvmap.frag
)

# Define dependencies for each shader file
set(SHADER_DEPENDENCIES
  "${ShaderFolder}/pbr.vert|${ShaderFolder}/shadermaterial.glsl"
  "${ShaderFolder}/pbr.vert|${ShaderFolder}/srgbtolinear.glsl"
  "${ShaderFolder}/pbr.vert|${ShaderFolder}/tonemapping.glsl"
)

# Function to get dependencies for a shader file
function(get_shader_dependencies shader_file result)
    set(dependencies ${shader_file})
    foreach(dep IN LISTS SHADER_DEPENDENCIES)
        string(REPLACE "|" ";" dep_list ${dep})
        list(GET dep_list 0 dep_shader_file)
        list(GET dep_list 1 dep_file)
        if(${dep_shader_file} STREQUAL ${shader_file})
            list(APPEND dependencies ${dep_file})
        endif()
    endforeach()
    set(${result} ${dependencies} PARENT_SCOPE)
endfunction()

# Group include files into "Include" folder in Solution Explorer
set(SHADERINCLUDE_FILES "")
foreach(dep IN LISTS SHADER_DEPENDENCIES)
    string(REPLACE "|" ";" dep_list ${dep})
    list(GET dep_list 1 include_file)
    list(APPEND SHADERINCLUDE_FILES ${include_file})
endforeach()
foreach(dep IN LISTS SHADERINCLUDE_FILES)
    message(STATUS "Dependency: ${dep}")
endforeach()

add_executable(app
  AppSupport.cpp
  DeviceCoordApp.cpp
  GeneratedTexturesApp.cpp
  gltfTerrainApp.cpp
  gltfObjectsApp.cpp
  BillboardDemo.cpp
  LandscapeDemo1.cpp
  LandscapeGenerator.cpp
  LineApp.cpp
  SimpleApp.cpp
  Incoming.cpp
  TextureViewer.cpp
  Loader.cpp
  SimpleMultiApp.cpp
  SimpleMultiWin.cpp
  ShadedPathV.cpp
  ${VULKAN_SHADERS}
  ${SHADERINCLUDE_FILES}  # Add include files to the target
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

target_link_libraries(app PRIVATE shadedpath)
target_precompile_headers(app REUSE_FROM shadedpath)

# shader compile take from openxr tutorial:
source_group("Shaders" FILES ${VULKAN_SHADERS})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shader.bin)
source_group("ShaderIncludes" FILES ${SHADERINCLUDE_FILES})

function(compile_glsl run_target_name)
    set(glsl_output_files "")
    foreach(in_file IN LISTS ARGN)
        #get_filename_component(glsl_stage ${in_file} NAME_WE)
        get_filename_component(glsl_stage ${in_file} NAME)
        set(out_file ${CMAKE_CURRENT_BINARY_DIR}/shader.bin/${glsl_stage}.spv)

        # Get dependencies for the current shader file
        get_shader_dependencies(${in_file} dependencies)

        # Run glslc if we can find it
        message(STATUS "command: Vulkan::glslc in: ${in_file} -o out: ${out_file}")
        # -g option is debug info
        # -O option is optimization
        add_custom_command(
            OUTPUT ${out_file}
            COMMAND Vulkan::glslc -O ${in_file} -o ${out_file}
            DEPENDS ${dependencies}
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
