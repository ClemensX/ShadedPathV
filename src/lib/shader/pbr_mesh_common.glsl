
// mesh info structures in large GPU mesh data block:
// we first have an index structure, where for every LOD level we index into the mesh info data array
// usually, LOD levels are just in increasing indices, but with using an index for each level we may change that any time
struct GPUMeshIndex {
    uint gpuMeshInfoIndex[10]; // index into mesh info array for LOD 0..9
};

struct GPUMeshInfo {
	uint64_t meshletOffset; // offset into global mesh storage buffer
	uint64_t localIndexOffset; // offset into global mesh storage buffer
	uint64_t globalIndexOffset; // offset into global mesh storage buffer
	uint64_t vertexOffset; // offset into global mesh storage buffer
};


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

// interpolated values (mesh -> frag shader)
struct PBRVertexOut {
    vec3 worldPos;
    float pad0; // strange that we need padding for structure passed from mesh to frag shader...
    vec3 normal;
    float pad1;
    vec2 uv0;
    vec2 uv1;
    //uvec4 joint0;
    vec4 weight0;
    vec4 color0;
};

// non interpolated values (mesh -> frag shader)
struct PBRVertexOutFlat {
    uvec4 joint0;
};

// make sure structure matches shaderValuesParams in pbrShader.h
struct UBOParams {
	vec4 lightDir;
    vec4 lightColor;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
	float intensity;
	int type; // 0=directional, 1=point, 2=spot
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

const uint MODEL_RENDER_FLAG_NONE              = 0u;
const uint MODEL_RENDER_FLAG_USE_VERTEX_COLORS = 1u << 0; // 0b0001
const uint MODEL_RENDER_FLAG_DISABLE           = 1u << 1; // 0b0010
// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
// one element of the large object material buffer (descriptor updated for each model group before rendering)
layout (binding = 1) uniform UboInstance {
    mat4 model; 
    mat4 jointMatrix[MAX_NUM_JOINTS];
    uint jointcount;
    uint flags; // see flag definitions above
    uint meshletsCount;
	uint objectNum; // object render mode: 1 == use vertex color only, 0 == regular BPR rendering
    PBRTextureIndexes indexes;
    UBOParams params[MAX_DYNAMIC_LIGHTS];
    ShaderMaterial material;
    BoundingBox boundingBox; // AABB axis aligned bounding box
    uint meshNumber;
    uint64_t GPUMeshStorageBaseAddress; // base address of global mesh storage buffer on GPU
	uint64_t meshletOffset; // offset into global mesh storage buffer
	uint64_t localIndexOffset; // offset into global mesh storage buffer
	uint64_t globalIndexOffset; // offset into global mesh storage buffer
	uint64_t vertexOffset; // offset into global mesh storage buffer
} model_ubo;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 baseColor;
    vec3 camPos;
} ubo;


layout(buffer_reference, std430) buffer VertexBuffer {
    PBRVertex vertex[];
};

layout(buffer_reference, std430) buffer MeshletDescs {
    uvec4 packedMeshlets[];
};

// local primitive index buffer:
layout(buffer_reference, std430) buffer PrimitiveIndexBuffer {
    uint primitiveIndices[];
};

// global index buffer:
layout(buffer_reference, std430) buffer GlobalIndexBuffer {
    uint index[];
};

// mesh infos on GPU global buffer:
layout(buffer_reference, std430) buffer GPUMeshIndexBuffer {
    GPUMeshIndex index[];
};

layout(buffer_reference, std430) buffer GPUMeshInfoBuffer {
    GPUMeshInfo info[];
};

// see pbrShader.h
layout(push_constant) uniform PushConstants {
	uint64_t meshStorageBufferAddress; // we name it differently here to make clear that this is (also) start address of memory chunk
	uint64_t baseAddressInfos;
} pushConstants;

// sentinel value used to signal culled/disabled from task -> mesh shader
const uint PAYLOAD_CULLED = 0xFFFFFFFFu;

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

// Unpack a 48-bit packed bounding box (6 x 8-bit quantized components).
// Layout (bits): [min.x(0..7), min.y(8..15), min.z(16..23), max.x(24..31), max.y(32..39), max.z(40..47)]
// Call with the two 32-bit words that contain the 48-bit value (low, high).
// sceneMin/sceneMax must be the same values used when packing on the CPU.
vec3 dequantizeByte(uint q, vec3 sceneMin, vec3 sceneMax) {
    float normalized = float(q) / 255.0;
    return sceneMin + normalized * (sceneMax - sceneMin);
}

void unpackBoundingBox48_from_uvec2(uvec2 packedLowHigh, vec3 sceneMin, vec3 sceneMax, out vec3 outMin, out vec3 outMax) {
    uint low = packedLowHigh.x;
    uint high = packedLowHigh.y;

    uint b0 = low & 0xFFu;            // min.x
    uint b1 = (low >> 8) & 0xFFu;     // min.y
    uint b2 = (low >> 16) & 0xFFu;    // min.z
    uint b3 = (low >> 24) & 0xFFu;    // max.x
    uint b4 = high & 0xFFu;           // max.y (bits 32..39)
    uint b5 = (high >> 8) & 0xFFu;    // max.z (bits 40..47)

    outMin = vec3(
        dequantizeByte(b0, sceneMin, sceneMax).x,
        dequantizeByte(b1, sceneMin, sceneMax).y,
        dequantizeByte(b2, sceneMin, sceneMax).z
    );

    outMax = vec3(
        dequantizeByte(b3, sceneMin, sceneMax).x,
        dequantizeByte(b4, sceneMin, sceneMax).y,
        dequantizeByte(b5, sceneMin, sceneMax).z
    );
}

// Convenience overload for a uvec4 where first two components hold the packed bounding box:
// e.g. if your meshlet descriptor is stored in a uvec4, pass that uvec4 directly.
void unpackBoundingBox48_from_uvec4(uvec4 packed4, vec3 sceneMin, vec3 sceneMax, out vec3 outMin, out vec3 outMax) {
    unpackBoundingBox48_from_uvec2(uvec2(packed4.x, packed4.y), sceneMin, sceneMax, outMin, outMax);
}