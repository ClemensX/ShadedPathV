#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout (points) in;
layout( triangle_strip, max_vertices = 4 ) out;

void main()
{
    vec4 inP = gl_in[0].gl_Position;
    debugPrintfEXT("geom.input x y z is %f %f %f\n", inP.x, inP.y, inP.z);
}