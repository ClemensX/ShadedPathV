struct BoundingBox {
	vec3 min;
    float pad0; // padding to align to vec4
	vec3 max;
    float pad1; // padding to align to vec4
};

const uint MODEL_RENDER_FLAG_NONE              = 0u;
const uint MODEL_RENDER_FLAG_USE_VERTEX_COLORS = 1u << 0; // 1
const uint MODEL_RENDER_FLAG_DISABLE           = 1u << 1; // 2
const uint MODEL_RENDER_FLAG_GPU_LOD           = 1u << 2; // 4, enable GPU LOD object manipulation

// see pbrShader.h
struct GPUMemoryAddressConstants {
    uint64_t collectionIndicesAddress;
    uint64_t collectionInfosAddressX;
    uint64_t meshInfosAddress;
    uint64_t modelsAddress;
    uint64_t modelsMovingAddress;
    uint64_t materialsAddress;
    uint64_t frameParamsAddress;
};

// see Object.h for C++ side
struct GPUCollectionIndex {
    uint gpuCollectionInfoIndex; // index into CollectionInfos (== index of first major mesh of this collection)
	uint mainMeshCount; // number of main meshes in this collection
};

struct GPUMeshInfo {
	uint64_t meshletOffset; // offset into global mesh storage buffer
	uint64_t localIndexOffset; // offset into global mesh storage buffer
	uint64_t globalIndexOffset; // offset into global mesh storage buffer
	uint64_t vertexOffset; // offset into global mesh storage buffer
	uint meshletCount; // number of meshlets for this LOD
    uint material; // during parsing: local material index, during GPU upload: global material index
	uint index; // global mesh index
	uint next; // next primitive (0 == no next primitive)
    BoundingBox boundingBox;
};

struct GPUModel {
	mat4 model;
	uint flags;
	uint meshNumber; // link to MeshInfo
	uint material_lod_category;
	uint materialIndex; // index into global material array
	//BoundingBox boundingBox;
};

struct GPUMaterial {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;

	float workflow;

    // indices into global texture array, -1 if not used
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;
	int emissiveTextureSet;
	int brdflutXXX;
	int irradianceXXX;
	int envcubeXXX;

	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	float emissiveStrength;

	uint lod_category;
	uint pad0;

    // texture coordinate sets, 0 or 1
	uint coord_set_baseColor;
	uint coord_set_metallicRoughness;
	uint coord_set_specularGlossiness;
	uint coord_set_normal;
	uint coord_set_occlusion;
	uint coord_set_emissive;
	bool isDoubleSided;
	uint pad1; // 4 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!
};

struct GPUFrameParam {
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
	int brdflut;
	int irradiance;
	int envcube;
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

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 baseColor;
	uint frameNum;      // new: current frame number
	uint pad0;          // pad to 16-byte multiple if desired (optional)
	uint pad1;          // pad to 16-byte multiple if desired (optional)
	uint pad2;          // pad to 16-byte multiple if desired (optional)
    GPUMemoryAddressConstants gpuMem;
    vec3 camPos;
} ubo;

// mesh infos on GPU global buffer:
layout(buffer_reference, std430) buffer GPUCollectionIndexBuffer {
    GPUCollectionIndex index[];
};

layout(buffer_reference, std430) buffer GPUMeshInfoBuffer {
    GPUMeshInfo info[];
};

layout(buffer_reference, std430) buffer GPUModelBuffer {
    GPUModel model[];
};

layout(buffer_reference, std430) buffer GPUMaterialBuffer {
    GPUMaterial material[];
};

layout(buffer_reference, std430) buffer GPUFrameParamBuffer {
    GPUFrameParam frameParam[];
};

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

layout(push_constant) uniform PushConstants {
	uint objectNum;  // 4 bytes only!
	uint pad0;          // pad to 16-byte multiple if desired (optional)
	uint pad1;          // pad to 16-byte multiple if desired (optional)
	uint pad2;          // pad to 16-byte multiple if desired (optional)
} pushConstants;

// sentinel value used to signal culled/disabled from task -> mesh shader
const uint PAYLOAD_CULLED = 0xFFFFFFFFu;

struct TaskPayload {
    mat4 mvp;
    uint meshIndex; // LOD selection
    uint meshletIndex; // only used for discarding
    // single meshlet to draw DO NOT iterate meshletIndex, emit multiple mesh shader calls at once with EmitMeshTasksEXT(meshletsCount, 1, 1);
};

GPUCollectionIndexBuffer gpuIndices = GPUCollectionIndexBuffer(ubo.gpuMem.collectionIndicesAddress + 0);
GPUMeshInfoBuffer gpuInfos = GPUMeshInfoBuffer(ubo.gpuMem.meshInfosAddress + 0);
GPUModelBuffer gpuModels = GPUModelBuffer(ubo.gpuMem.modelsAddress + 0);
GPUModelBuffer gpuModelsMoving = GPUModelBuffer(ubo.gpuMem.modelsMovingAddress + 0);
GPUMaterialBuffer gpuMaterials = GPUMaterialBuffer(ubo.gpuMem.materialsAddress + 0);
GPUFrameParamBuffer gpuFrameParams = GPUFrameParamBuffer(ubo.gpuMem.frameParamsAddress + 0);

// utility functions

// calc obj position as middle of bounding bos in world coords, diameter is returned in .w component of return value
vec4 calcRealObjectPosition(BoundingBox bb, mat4 mvp) {
    vec3 realObjPos;
    // get BB in world coords:
    BoundingBox bbWorld = bb;
    vec4 bbMinWorld = mvp * vec4(bb.min, 1.0);
    vec4 bbMaxWorld = mvp * vec4(bb.max, 1.0);
    bbWorld.min = bbMinWorld.xyz / bbMinWorld.w;
    bbWorld.max = bbMaxWorld.xyz / bbMaxWorld.w;
    // diameter is always bb max - bb min
    float diameter = length(bbWorld.max - bbWorld.min);
    //debugPrintfEXT("TASK SHADER: BB %f %f %f --> %f %f %f\n", bbWorld.min.x, bbWorld.min.y, bbWorld.min.z, bbWorld.max.x, bbWorld.max.y, bbWorld.max.z);
    //if (model_ubo.objectNum == 0) debugPrintfEXT("TASK SHADER: object %u calc diameter %f , flags %d\n", model_ubo.objectNum, diameter, model_ubo.flags);
    // calc object position as middle of BB:
    realObjPos = (bbWorld.min + bbWorld.max) * 0.5f;
    // log realObjPos:
    //debugPrintfEXT("UTIL: realObjPos %f %f %f, diameter %f\n", realObjPos.x, realObjPos.y, realObjPos.z, diameter);
    return vec4(realObjPos, diameter);
}

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
// info and debug methods

void printGPUBufferAddresses() {
    debugPrintfEXT("PBR TASK SHADER PUUUUSH Buffer addresses:\n  %llx \n  meshInfosAddress %llx\n", ubo.gpuMem.collectionIndicesAddress, ubo.gpuMem.meshInfosAddress);
    debugPrintfEXT("  modelsAddress %llx\n", ubo.gpuMem.modelsAddress);
    debugPrintfEXT("  modelsMovingAddress %llx\n", ubo.gpuMem.modelsMovingAddress);
    debugPrintfEXT("  materialsAddress %llx\n", ubo.gpuMem.materialsAddress);
    debugPrintfEXT("  frameParamsAddress %llx\n", ubo.gpuMem.frameParamsAddress);
}

void printGPUMeshInfo(GPUMeshInfo info) {
    debugPrintfEXT("GPUMeshInfo: meshletOffset %llx localIndexOffset %llx globalIndexOffset %llx vertexOffset %llx meshletCount %u material %u index %u next %u\n",
        info.meshletOffset, info.localIndexOffset, info.globalIndexOffset, info.vertexOffset, info.meshletCount, info.material, info.index, info.next);
    debugPrintfEXT("  BB: min %f %f %f max %f %f %f\n", info.boundingBox.min.x, info.boundingBox.min.y, info.boundingBox.min.z, info.boundingBox.max.x, info.boundingBox.max.y, info.boundingBox.max.z);
}
void printGPUCollectionIndex(uint index) {
    GPUCollectionIndex idx = gpuIndices.index[index];
    debugPrintfEXT("GPUCollectionIndex %u: gpuCollectionInfoIndex %u mainMeshCount %u\n", index, idx.gpuCollectionInfoIndex, idx.mainMeshCount);
}

void printVertex( PBRVertex v) {
    debugPrintfEXT("PBRVertex: pos %f %f %f normal %f %f %f uv0 %f %f uv1 %f %f\n", v.position.x, v.position.y, v.position.z, v.normal.x, v.normal.y, v.normal.z, v.uv0.x, v.uv0.y, v.uv1.x, v.uv1.y);
}

void verifyModel(uint index) {
    debugPrintfEXT("verify model %u:\n", index);
    GPUModel model = gpuModels.model[index];
    debugPrintfEXT("GPUModel: flags %u meshNumber %u material_lod_category %u materialIndex %u\n",
        model.flags, model.meshNumber, model.material_lod_category, model.materialIndex);
    debugPrintfEXT("   transform: %f %f %f %f\n", model.model[0][0], model.model[0][1], model.model[0][2], model.model[0][3]);
    debugPrintfEXT("   transform: %f %f %f %f\n", model.model[1][0], model.model[1][1], model.model[1][2], model.model[1][3]);
    debugPrintfEXT("   transform: %f %f %f %f\n", model.model[2][0], model.model[2][1], model.model[2][2], model.model[2][3]);
    debugPrintfEXT("   transform: %f %f %f %f\n", model.model[3][0], model.model[3][1], model.model[3][2], model.model[3][3]);
}

void verifyMaterial(uint index) {
    debugPrintfEXT("verify material %u:\n", index);
    GPUMaterial material = gpuMaterials.material[index];
    debugPrintfEXT("GPUMaterial: baseColorTextureSet %d physicalDescriptorTextureSet %d normalTextureSet %d occlusionTextureSet %d emissiveTextureSet %d",
        material.baseColorTextureSet, material.physicalDescriptorTextureSet, material.normalTextureSet, material.occlusionTextureSet, material.emissiveTextureSet);
    debugPrintfEXT("\n  lod: %d", material.lod_category);
    debugPrintfEXT("\n  alphaMask: %f", material.alphaMask);
    //debugPrintfEXT("\n  brdflut: %d irradiance: %d envcube: %d", material.brdflut, material.irradiance, material.envcube);
    debugPrintfEXT("\n");
}

void verifyMesh(uint index) {
    debugPrintfEXT("verify mesh %u:\n", index);
    GPUMeshInfo info = gpuInfos.info[index];
    printGPUMeshInfo(info);
    VertexBuffer vertices = VertexBuffer(info.vertexOffset);
    for (int i = 0; i < 24; i++) {
        PBRVertex v = vertices.vertex[i];
        printVertex(v);
        //debugPrintfEXT("Vertex %d: pos %f %f %f normal %f %f %f uv0 %f %f uv1 %f %f\n", i, v.pos.x, v.pos.y, v.pos.z, v.normal.x, v.normal.y, v.normal.z, v.uv0.x, v.uv0.y, v.uv1.x, v.uv1.y);
    }
}

void verifyFrameParam(uint index) {
    debugPrintfEXT("verify frame param %u:\n", index);
    GPUFrameParam param = gpuFrameParams.frameParam[index];
    debugPrintfEXT("GPUFrameParam: lightDir %f %f %f %f lightColor %f %f %f %f\n  exposure %f gamma %f prefilteredCubeMipLevels %f scaleIBLAmbient %f\n  debugViewInputs %f debugViewEquation %f intensity %f type %d\n",
        param.lightDir.x, param.lightDir.y, param.lightDir.z, param.lightDir.w,
        param.lightColor.x, param.lightColor.y, param.lightColor.z, param.lightColor.w,
        param.exposure, param.gamma, param.prefilteredCubeMipLevels, param.scaleIBLAmbient,
        param.debugViewInputs, param.debugViewEquation, param.intensity, param.type);
    debugPrintfEXT("  brdflut %d irradiance %d envcube %d\n", param.brdflut, param.irradiance, param.envcube);
}
