#version 450
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;


layout(location = 0) out vec3 fragColor;

void main() {
    debugPrintfEXT("My float is %f\n", ubo.proj[0][0]);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    debugPrintfEXT("final device coord: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
}