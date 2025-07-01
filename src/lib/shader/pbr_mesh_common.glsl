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


struct TaskPayload {
    mat4 mvp;
    uint meshletIndex;
    // Add more fields as needed (e.g., LOD, culling flags, etc.)
};

// utility functions

// check if object AABB is completely outside view frustrum
bool isOutsideView(BoundingBox bb, mat4 mvp) {
    vec3 aabbMin = bb.min;
    vec3 aabbMax = bb.max;

    // generate 8 corners of AABB:
    vec3 corners[8];
    corners[0] = vec3(aabbMin.x, aabbMin.y, aabbMin.z);
    corners[1] = vec3(aabbMax.x, aabbMin.y, aabbMin.z);
    corners[2] = vec3(aabbMin.x, aabbMax.y, aabbMin.z);
    corners[3] = vec3(aabbMax.x, aabbMax.y, aabbMin.z);
    corners[4] = vec3(aabbMin.x, aabbMin.y, aabbMax.z);
    corners[5] = vec3(aabbMax.x, aabbMin.y, aabbMax.z);
    corners[6] = vec3(aabbMin.x, aabbMax.y, aabbMax.z);
    corners[7] = vec3(aabbMax.x, aabbMax.y, aabbMax.z);

    // Transform corners to clip space
    vec4 clipCorners[8];
    for (int i = 0; i < 8; ++i) {
        clipCorners[i] = mvp * vec4(corners[i], 1.0);
    }

    // For each plane, if all corners are outside, the object is outside the frustum:
    bool outside = false;
    for (int plane = 0; plane < 6; ++plane) {
        int outCount = 0;
        for (int i = 0; i < 8; ++i) {
            vec4 c = clipCorners[i];
            if (plane == 0 && c.x < -c.w) outCount++; // left
            if (plane == 1 && c.x >  c.w) outCount++; // right
            if (plane == 2 && c.y < -c.w) outCount++; // bottom
            if (plane == 3 && c.y >  c.w) outCount++; // top
            if (plane == 4 && c.z <  0.0) outCount++; // near (Vulkan)
            if (plane == 5 && c.z >  c.w) outCount++; // far
        }
        if (outCount == 8) {
            outside = true;
            break;
        }
    }

    return outside;
}

// return 1 or 2 if object outside view frustrum or pixel size too small
int isOutsideViewOrTooSmall(BoundingBox bb, mat4 mvp) {
    //const vec2 viewParams_screenSize = vec2(4096.0, 4096.0);
    const vec2 viewParams_screenSize = vec2(1800.0, 1017.0);
    vec3 aabbMin = bb.min;
    vec3 aabbMax = bb.max;

    // generate 8 corners of AABB:
    vec3 corners[8];
    corners[0] = vec3(aabbMin.x, aabbMin.y, aabbMin.z);
    corners[1] = vec3(aabbMax.x, aabbMin.y, aabbMin.z);
    corners[2] = vec3(aabbMin.x, aabbMax.y, aabbMin.z);
    corners[3] = vec3(aabbMax.x, aabbMax.y, aabbMin.z);
    corners[4] = vec3(aabbMin.x, aabbMin.y, aabbMax.z);
    corners[5] = vec3(aabbMax.x, aabbMin.y, aabbMax.z);
    corners[6] = vec3(aabbMin.x, aabbMax.y, aabbMax.z);
    corners[7] = vec3(aabbMax.x, aabbMax.y, aabbMax.z);

    vec2 minScreen = vec2(1e10);
    vec2 maxScreen = vec2(-1e10);
    // Transform corners to clip space
    vec4 clipCorners[8];
    for (int i = 0; i < 8; ++i) {
        vec4 clip = mvp * vec4(corners[i], 1.0);
        clipCorners[i] = clip;
        if (clip.w <= 0.0) continue;
        vec3 ndc = clip.xyz / clip.w;
        vec2 screen = (ndc.xy * 0.5 + 0.5) * viewParams_screenSize;
        minScreen = min(minScreen, screen);
        maxScreen = max(maxScreen, screen);
    }
    float pixelWidth  = maxScreen.x - minScreen.x;
    float pixelHeight = maxScreen.y - minScreen.y;
    float maxPixelSize = max(pixelWidth, pixelHeight);

    //debugPrintfEXT("maxPixelSize %f\n", maxPixelSize);
    if (maxPixelSize < 5.0) {
        // Cull object
        return 2;
    }

    // For each plane, if all corners are outside, the object is outside the frustum:
    bool outside = false;
    for (int plane = 0; plane < 6; ++plane) {
        int outCount = 0;
        for (int i = 0; i < 8; ++i) {
            vec4 c = clipCorners[i];
            if (plane == 0 && c.x < -c.w) outCount++; // left
            if (plane == 1 && c.x >  c.w) outCount++; // right
            if (plane == 2 && c.y < -c.w) outCount++; // bottom
            if (plane == 3 && c.y >  c.w) outCount++; // top
            if (plane == 4 && c.z <  0.0) outCount++; // near (Vulkan)
            if (plane == 5 && c.z >  c.w) outCount++; // far
        }
        if (outCount == 8) {
            outside = true;
            break;
        }
    }

    if (outside) return 1;
    return 0; // visible and big enough
}

//    int isOutside = isOutsideViewOrTooSmall(model_ubo.boundingBox, mvp);
//    if (isOutside == 1) {
//        // Don't emit mesh tasks for this object
//        BoundingBox bb = model_ubo.boundingBox;
//        debugPrintfEXT("WARNING: OUTSIDE bb min %f %f %f\n", bb.min.x, bb.min.y, bb.min.z);
//        return;
//    } else if (isOutside == 2) {
//        // Don't emit mesh tasks for this object
//        BoundingBox bb = model_ubo.boundingBox;
//        debugPrintfEXT("WARNING: TOO SMALL bb min %f %f %f\n", bb.min.x, bb.min.y, bb.min.z);
//        return;
//    }
//