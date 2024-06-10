#version 450
// enable dynamic indexing:
// https://github.com/KhronosGroup/MoltenVK/issues/1696
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_KHR_vulkan_glsl:enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform sampler2D global_textures[];

void main() {
    //outColor = vec4(fragTexCoord, 0.0, 1.0); fake texture with colors
    //outColor = texture(texSampler, fragTexCoord);

    //use global texture array:
    //outColor = texture(global_textures[3], fragTexCoord);

    // mix textures:
    uint baseIndex = 0;
    if (fragTexCoord.x > 0.5) baseIndex = 2;
    outColor = texture(global_textures[nonuniformEXT(baseIndex)], fragTexCoord);
}