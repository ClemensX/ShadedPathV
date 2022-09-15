#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inDirection;
layout(location = 2) in float inWidth;
layout(location = 3) in float inHeight;
//layout(location = 1) out float PointSize;

void main()
{
    gl_PointSize = 1.0;
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
}
