#version 460
#extension GL_EXT_debug_printf : enable
//#extension GL_EXT_mesh_shader : enable
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in uvec4 inJoint0;
layout (location = 5) in vec4 inWeight0;
layout (location = 6) in vec4 inColor0;

//struct UBOParams {
//	vec4 lightDir;
//	float exposure;
//	float gamma;
//	float prefilteredCubeMipLevels;
//	float scaleIBLAmbient;
//	float debugViewInputs;
//	float debugViewEquation;
//	float pad0;
//	float pad1;
//};
//
#include "common_cpp_shader.h"
#include "shadermaterial.glsl"
#include "pbr_mesh_common.glsl"

//layout(binding = 0) uniform UniformBufferObject {
//    mat4 model;
//    mat4 view;
//    mat4 proj;
//    vec4 baseColor;
//    vec3 camPos;
//} ubo;
//
//struct PBRTextureIndexes {
//    uint baseColor;
//    uint metallicRoughness;
//    uint normal;
//    uint occlusion;
//    uint emissive;
//    uint pad0;
//    uint pad1;
//    uint pad2;
//};
//
//#define MAX_NUM_JOINTS 128
//
// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
// one element of the large object material buffer (descriptor updated for each model group before rendering)
//layout (binding = 1) uniform UboInstance {
//    mat4 model; 
//    mat4 jointMatrix[MAX_NUM_JOINTS];
//    uint jointcount;
//    uint flags;
//    uint pad1;
//    uint pad2;
//    //uint padding[2]; // 8 bytes of padding to align the next member to 16 bytes
//    PBRTextureIndexes indexes;
//    UBOParams params;
//    ShaderMaterial material;
//} model_ubo;
// 

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;
layout (location = 4) out vec4 outColor0;
layout (location = 5) out uint mode_out;
layout (location = 6) out vec3 outCamPos;

// sync with pbrPushConstants in pbrShader.h
//layout(push_constant) uniform pbrPushConstants {
//    uint mode; // 0: standard pbr metallicRoughness, 1: pre-light vertices with color in vertex structure
//} push;
//

layout(std430, binding = 2) buffer MeshletBuffer {
    uvec4 packedMeshlets[];
} meshletDescs;

void check_inputs() {
    uvec4 uv = meshletDescs.packedMeshlets[0];
    //debugPrintfEXT("meshlet uvec4[0] %f %f %f %f\n", uv.x, uv.y, uv.z, uv.w);
    MeshletDesc meshlet = unpackMeshletDesc(uv);
    uint indexBufferOffset = meshlet.indexBufferOffset;
    //debugPrintfEXT("meshlet indexBufferOffset %d\n", indexBufferOffset);
    vec4 v = inColor0;
    vec3 x;
    vec2 c;
    uint flags = model_ubo.flags;
    //debugPrintfEXT("model_ubo flags %d\n", flags);
    //debugPrintfEXT("inColor0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
    v = ubo.model[0];
    //debugPrintfEXT("model %f %f %f %f\n", v.x, v.y, v.z, v.w);
    v = model_ubo.model[0];
    //debugPrintfEXT("model_ubo %f %f %f %f\n", v.x, v.y, v.z, v.w);
    v = ubo.view[0];
    //debugPrintfEXT("view %f %f %f %f\n", v.x, v.y, v.z, v.w);
    v = ubo.proj[0];
    //debugPrintfEXT("proj %f %f %f %f\n", v.x, v.y, v.z, v.w);
    x = inPos;
    //debugPrintfEXT("inPos %f %f %f\n", x.x, x.y, x.z);
    x = inNormal;
    //debugPrintfEXT("inNormal %f %f %f\n", x.x, x.y, x.z);
    c = inUV0;
    //debugPrintfEXT("inUV0 %f %f\n", c.x, c.y);
    c = inUV1;
    //debugPrintfEXT("inUV1 %f %f\n", c.x, c.y);
    float f = model_ubo.params[0].gamma;
    //debugPrintfEXT("uboParams.gamma %f\n", f);
    f = model_ubo.material.alphaMask;
    //debugPrintfEXT("material alphaMask %f\n", f);
    int b = model_ubo.material.brdflut;
    int e = model_ubo.material.envcube;
    //debugPrintfEXT("material brdflut envcube %d %d\n", b, e);
    x = inPos;
	if (x.y >= 0.812845) {
		//debugPrintfEXT("vert pos %f %f %f\n", x.x, x.y, x.z);
	}
}

void main() {
    //check_inputs();
    //outParams = model_ubo.params;
    //outMaterial = model_ubo.material;
    //textureIndexes = model_ubo.indexes;
    //textureIndexes.baseColor = model_ubo.jointcount;
    //textureIndexes.baseColor = model_ubo.indexes.baseColor;
    //debugPrintfEXT("ubo.model tex index is %d\n", model_ubo.indexes.emissive);
    //debugPrintfEXT("ubo.model tex index is %d %d %d %d\n", model_ubo.jointcount, model_ubo.pad0, model_ubo.pad1, model_ubo.pad2);

//    baseColorIndex = model_ubo.indexes.baseColor;
//    metallicRoughnessIndex = model_ubo.indexes.metallicRoughness;
//    normalIndex = model_ubo.indexes.normal;
//    occlusionIndex = model_ubo.indexes.occlusion;
//    emissiveIndex = model_ubo.indexes.emissive;
//
//    test indexes
//    baseColorIndex = model_ubo.indexes.metallicRoughness;
//    metallicRoughnessIndex = model_ubo.indexes.baseColor;

	outColor0 = inColor0;
    mode_out = 0;//push.mode;
    outCamPos = ubo.camPos;

    vec4 locPos;
	locPos = model_ubo.model * vec4(inPos, 1.0);
	outNormal = normalize(transpose(inverse(mat3(model_ubo.model))) * inNormal);
	//locPos.y = -locPos.y;
	outWorldPos = locPos.xyz / locPos.w;
	outUV0 = inUV0;
	outUV1 = inUV1;
    //debugPrintfEXT("loc.w %f\n", locPos.w);
    if ((model_ubo.flags & MODEL_RENDER_FLAG_USE_VERTEX_COLORS) != 0) { // if model_ubo.flags & MODEL_FLAG_PRELIGHT_VERTICES
    // bit 0 is set
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0); // W=0, will be clipped
    } else {
        gl_Position =  ubo.proj * ubo.view * vec4(outWorldPos, 1.0);
    }
}
