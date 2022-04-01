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
    //debugPrintfEXT("input vertex world coord: %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("ubo.view 0 0 is %f\n", ubo.view[0][0]);
    //debugPrintfEXT("ubo.proj 0 0 is %f\n", ubo.proj[0][0]);

    fragColor = inColor;
    //fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
}