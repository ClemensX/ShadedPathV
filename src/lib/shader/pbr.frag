#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_KHR_vulkan_glsl:enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

struct PBRTextureIndexes {
  uint baseColor;
};
layout(location = 2) flat in PBRTextureIndexes textureIndexes;


layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D global_textures[];

void main() {
    uint baseIndex = textureIndexes.baseColor;
    outColor = texture(global_textures[nonuniformEXT(baseIndex)], fragTexCoord);
    // discard transparent pixels (== do not write z-buffer)
    if (outColor.w < 0.8) {
        discard;
    }
}