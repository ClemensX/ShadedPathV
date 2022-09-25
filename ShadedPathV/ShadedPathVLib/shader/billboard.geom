#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout (points) in;
layout( triangle_strip, max_vertices = 4 ) out;

void main()
{
    vec4 inP = gl_in[0].gl_Position;
    //debugPrintfEXT("bb geom.input x y z is %f %f %f\n", inP.x, inP.y, inP.z);

    vec4 v0, v1, v2; // assemble vertex info here
    v0 = vec4(0, 0, 0.5, 1);
    v1 = vec4(0.5, 0.5, 0.5, 1);
    v2 = vec4(0, 0.5, 0.5, 1);
    gl_Position = v0;
    EmitVertex();
    gl_Position = v2;
    EmitVertex();
    gl_Position = v1;
    EmitVertex();
}