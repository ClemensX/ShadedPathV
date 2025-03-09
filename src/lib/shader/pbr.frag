#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : disable

////layout(location = 0) in vec3 fragColor;
//layout(location = 1) in vec2 fragTexCoord;
//layout(location = 0) in vec4 vertexColor;
//layout(location = 3) flat in uint mode;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) flat in uint baseColorIndex;
layout (location = 6) flat in uint metallicRoughnessIndex;
layout (location = 7) flat in uint normalIndex;
layout (location = 8) flat in uint occlusionIndex;
layout (location = 9) flat in uint emissiveIndex;
layout (location = 10) flat in uint mode;


struct PBRTextureIndexes {
  uint baseColor;
};

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D global_textures[];

void main() {
    //debugPrintfEXT("pbr frag render mode: %d\n", mode);
    //uint baseIndex = metallicRoughnessIndex;//baseColorIndex; // test indexes
    uint baseIndex = baseColorIndex;
    //if (occlusionIndex == -1) { // just a test :-)
    //    baseIndex = 0;
    //}
    if (mode == 1) {
		outColor = inColor0;
	} else {
        outColor = texture(global_textures[nonuniformEXT(baseIndex)], inUV0) * inColor0;
    }
    // discard transparent pixels (== do not write z-buffer)
    if (outColor.w < 0.8) {
        discard;
    }
}