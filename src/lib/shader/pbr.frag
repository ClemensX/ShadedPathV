#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor0;
layout(location = 4) flat in uint mode;

struct PBRTextureIndexes {
  uint baseColor;
};
layout(location = 2) flat in PBRTextureIndexes textureIndexes;


layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D global_textures[];

void main() {
    //debugPrintfEXT("pbr frag render mode: %d\n", mode);
    uint baseIndex = textureIndexes.baseColor;
    if (mode == 1) {
		outColor = fragColor0;
	} else {
        outColor = texture(global_textures[nonuniformEXT(baseIndex)], fragTexCoord);
    }
    // discard transparent pixels (== do not write z-buffer)
    if (outColor.w < 0.8) {
        discard;
    }
}