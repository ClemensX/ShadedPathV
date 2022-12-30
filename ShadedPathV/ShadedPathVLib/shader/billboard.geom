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

layout(location = 1) in uint inTypes[]; // billboard type: 0 is towards camera, 1 is absolute inDirection
layout(location = 2) in vec4 inQuats[]; // quaternion for rotating vertices if type == 1

void main()
{
    uint inType = inTypes[0];
    vec4 quat = inQuats[0];
    vec4 inP = gl_in[0].gl_Position;
    //debugPrintfEXT("bb geom.input x y z is %f %f %f\n", inP.x, inP.y, inP.z);
    //debugPrintfEXT("bb geom ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("bb geom ubo.view 0 0 is %f\n", ubo.view[0][0]);

    vec4 v0, v1, v2; // assemble vertex info here
    v0 = vec4(-0.1, 0, 0, 1) + inP;
    v1 = vec4(0.1, 0, 0, 1) + inP;
    v2 = vec4(0, 0.1, 0, 1) + inP;
//    v0 = vec4(-0.1, 0, 0, 0) + inP;
//    v1 = vec4(0.1, 0, 0, 0) + inP;
//    v2 = vec4(-0.1, 0.1, 0, 0) + inP;
    if (inType == 0) {
        gl_Position = ubo.proj * v0;
        EmitVertex();
        gl_Position = ubo.proj * v1;
        EmitVertex();
        gl_Position = ubo.proj * v2;
        EmitVertex();
        EndPrimitive();
    } else if (inType == 1) {
        v0 = quat * v0;
        v1 = quat * v1;
        v2 = quat * v2;
        v0 = ubo.proj * ubo.view * ubo.model * v0;
        v1 = ubo.proj * ubo.view * ubo.model * v1;
        v2 = ubo.proj * ubo.view * ubo.model * v2;
        gl_Position = v0;
        EmitVertex();
        gl_Position = v1;
        EmitVertex();
        gl_Position = v2;
        EmitVertex();
        EndPrimitive();
    }
}