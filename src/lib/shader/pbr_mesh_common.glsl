// see https://github.com/nvpro-samples/gl_vk_meshlet_cadscene/blob/master/common.h
#ifndef NVMESHLET_VERTEX_COUNT
// primitive count should be 40, 84 or 126
//    vertex count should be 32 or 64
//
// 64 vertices &  84 triangles:
//    works typically well for NV
// 64 vertices &  64 triangles:
//    is more portable for EXT usage
//    (hw that does 128 & 128 well, can do 2 x 64 & 64 at once)
// 64 vertices & 126 triangles:
//    can work in z-only or other very low extra
//    vertex attribute scenarios for NV
//
#define NVMESHLET_VERTEX_COUNT 64
#define NVMESHLET_PRIMITIVE_COUNT 64
// must be multiple of SUBGROUP_SIZE
#define NVMESHLET_PER_TASK 32
#endif

#ifndef NVMESHLET_ENCODING
#define NVMESHLET_ENCODING NVMESHLET_ENCODING_PACKBASIC
#endif


// Meshlet descriptor struct and unpack function (as in your vertex shader)
struct MeshletDesc {
    uint boundingBoxLow;
    uint boundingBoxHigh;
    uint numVertices;
    uint numPrimitives;
    uint vertexPack;
    uint indexBufferOffset;
    uint normalCone;
};

MeshletDesc unpackMeshletDesc(uvec4 packed) {
    MeshletDesc desc;
    uint low0 = packed.x;
    uint low1 = packed.y;
    uint high0 = packed.z;
    uint high1 = packed.w;
    desc.boundingBoxLow  = low0;
    desc.boundingBoxHigh = low1 & 0xFFFF;
    desc.numVertices = (low1 >> 16) & 0xFF;
    desc.numPrimitives = (low1 >> 24) & 0xFF;
    desc.vertexPack = high0 & 0xFF;
    desc.indexBufferOffset = (high0 >> 8) | ((high1 & 0xFF) << 24);
    desc.normalCone = (high1 >> 8) & 0xFFFFFF;
    return desc;
}

struct PBRVertex {
    vec3 position;
    vec3 normal;
    vec2 uv0;
    vec2 uv1;
    uvec4 joint0;
    vec4 weight0;
    vec4 color0;
};

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

struct BoundingBox {
	vec3 min;
    float pad0; // padding to align to vec4
	vec3 max;
    float pad1; // padding to align to vec4
};

#define MAX_NUM_JOINTS 128

// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
// one element of the large object material buffer (descriptor updated for each model group before rendering)
layout (binding = 1) uniform UboInstance {
    mat4 model; 
    mat4 jointMatrix[MAX_NUM_JOINTS];
    uint jointcount;
    uint flags;
    uint meshletsCount;
    uint pad1;
    //uint padding[2]; // 8 bytes of padding to align the next member to 16 bytes
    PBRTextureIndexes indexes;
    UBOParams params;
    ShaderMaterial material;
    BoundingBox boundingBox; // AABB axis aligned bounding box
} model_ubo;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 baseColor;
    vec3 camPos;
} ubo;


