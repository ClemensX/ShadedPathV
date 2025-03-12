#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

struct UBOParams {
	vec4 lightDir;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
	float pad0;
	float pad1;
};

#include "shadermaterial.glsl"

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
// mode 0: pbr metallic roughness
// mode 1: only use vertex color

struct PBRTextureIndexes {
    uint baseColor;
    uint metallicRoughness;
    uint normal;
    uint occlusion;
    uint emissive;
    uint pad0;
    uint pad1;
    uint pad2;
};

#define MAX_NUM_JOINTS 128

// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
// one element of the large object material buffer (descriptor updated for each model group before rednering)
layout (binding = 1) uniform UboInstance {
    mat4 model; 
    mat4 jointMatrix[MAX_NUM_JOINTS];
    uint jointcount;
    uint pad0;
    uint pad1;
    uint pad2;
    //uint padding[2]; // 8 bytes of padding to align the next member to 16 bytes
    PBRTextureIndexes indexes;
    UBOParams params;
    ShaderMaterial material;
} model_ubo;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube global_textures3d[];
layout(set = 1, binding = 0) uniform sampler2D global_textures2d[];

vec4 textureBindless3D(uint textureid, vec3 uv) {
	return texture(global_textures3d[nonuniformEXT(textureid)], uv);
}

vec4 textureBindless2D(uint textureid, vec2 uv) {
	return texture(global_textures2d[nonuniformEXT(textureid)], uv);
}

UBOParams uboParams = model_ubo.params;

void main() {
	ShaderMaterial material = model_ubo.material;
    float f = uboParams.gamma;
    //debugPrintfEXT("uboParams.gamma %f\n", f);
    f = material.roughnessFactor;
    //debugPrintfEXT("frag material.roughnessFactor %f\n", f);
    debugPrintfEXT("frag base set indexes direct / ubo:  %d / %d\n", baseColorIndex, material.baseColorTextureSet);

    f = uboParams.debugViewEquation;
    //debugPrintfEXT("frag uboParams.debugvieweq %f\n", f);
    f = uboParams.gamma;
    //debugPrintfEXT("frag uboParams.gamma %f\n", f);
    //debugPrintfEXT("pbr frag render mode: %d\n", mode);
    uint baseIndex = material.baseColorTextureSet;//emissiveIndex;//baseColorIndex; // test indexes
    //uint baseIndex = baseColorIndex;
    //if (occlusionIndex == -1) { // just a test :-)
    //    baseIndex = 0;
    //}
    if (mode == 1) {
		outColor = inColor0;
	} else {
        outColor = textureBindless2D(baseIndex, inUV0) * inColor0;
    }
    // discard transparent pixels (== do not write z-buffer)
    if (outColor.w < 0.8) {
        discard;
    }
}