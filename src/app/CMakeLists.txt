set(ShaderFolder ${PROJECT_SOURCE_DIR}/src/lib/shader)
file(GLOB VULKAN_SHADERS
  ${ShaderFolder}/billboard.frag
  ${ShaderFolder}/billboard.geom
  ${ShaderFolder}/billboard.vert
  ${ShaderFolder}/cube.frag
)

add_executable(app
  LandscapeGenerator.cpp
  ShadedPathV.cpp
  ${VULKAN_SHADERS}
)
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()
target_link_libraries(app PRIVATE shadedpath)
#add_custom_command(POST_BUILD
#add_custom_command(TARGET app POST_BUILD
#  OUTPUT line_vert.spv
#  COMMAND Vulkan::glslc ${PROJECT_SOURCE_DIR}/src/lib/shader/line.vert -o $<TARGET_FILE_DIR:app>/line_vert.spv
#  DEPENDS ${PROJECT_SOURCE_DIR}/src/lib/shader/line.vert
#)

#add_custom_command(
#  OUTPUT line_vert.spv
#  COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/shader/line.vert -o line_vert.spv
#  #DEPENDS ./shader/line.vert
#)

# shader compile take from openxr tutorial:
source_group("Shaders" FILES ${VULKAN_SHADERS})

function(compile_glsl run_target_name)
    set(glsl_output_files "")
    foreach(in_file IN LISTS ARGN)
        #get_filename_component(glsl_stage ${in_file} NAME_WE)
        get_filename_component(glsl_stage ${in_file} NAME)
        set(out_file ${CMAKE_CURRENT_BINARY_DIR}/${glsl_stage}.spv)
        #set(out_file ${CMAKE_CURRENT_BINARY_DIR}/${in_file}.spv)
            # Run glslc if we can find it
            add_custom_command(
                OUTPUT ${out_file}
                COMMAND ${GLSL_COMPILER} ${in_file} -o ${out_file}
                DEPENDS ${in_file}
            )
        list(APPEND glsl_output_files ${out_file})
    endforeach()
    add_custom_target(${run_target_name} ALL DEPENDS ${glsl_output_files})
    #set_target_properties(${run_target_name} PROPERTIES FOLDER ${HELPER_FOLDER})

endfunction()


compile_glsl(glsl_compiles ${VULKAN_SHADERS})

add_dependencies(app
    glsl_compiles
)
