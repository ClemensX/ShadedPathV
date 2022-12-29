#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout (points) in;
layout( triangle_strip, max_vertices = 4 ) out;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    vec4 inP = gl_in[0].gl_Position;
    debugPrintfEXT("bb geom.input x y z is %f %f %f\n", inP.x, inP.y, inP.z);
    //debugPrintfEXT("bb geom ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("bb geom ubo.view 0 0 is %f\n", ubo.view[0][0]);

    vec4 v0, v1, v2; // assemble vertex info here
    v0 = vec4(-0.1, 0, 0, 1) + inP;
    v1 = vec4(0.1, 0, 0, 1) + inP;
    v2 = vec4(0, 0.1, 0, 1) + inP;
//    v0 = vec4(-0.1, 0, 0, 0) + inP;
//    v1 = vec4(0.1, 0, 0, 0) + inP;
//    v2 = vec4(-0.1, 0.1, 0, 0) + inP;
    gl_Position = ubo.proj * v0;
    EmitVertex();
    gl_Position = ubo.proj * v1;
    EmitVertex();
    //gl_Position = /* ubo.proj * */v1;
    gl_Position = ubo.proj * v2;
    EmitVertex();
    EndPrimitive();
}