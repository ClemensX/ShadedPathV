#version 450
#extension GL_KHR_vulkan_glsl:enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout (set = 1, binding = 0) uniform sampler2D baseTex;

void main() {
    vec4 outColor2 = texture(baseTex, fragTexCoord);
    outColor = outColor2;
}