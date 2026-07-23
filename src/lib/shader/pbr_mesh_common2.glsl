struct BoundingBox {
	vec3 min;
    float pad0; // padding to align to vec4
	vec3 max;
    float pad1; // padding to align to vec4
};

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
	int baseColor;
	int metallicRoughness;
	int normal;
	int occlusion;
	int emissive;
    int isDoubleSided;
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

layout(push_constant) uniform PushConstants {
	uint objectNum;  // 4 bytes only!
	uint pad0;          // pad to 16-byte multiple if desired (optional)
	uint pad1;          // pad to 16-byte multiple if desired (optional)
	uint pad2;          // pad to 16-byte multiple if desired (optional)
} pushConstants;


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


// util methods

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
    debugPrintfEXT("GPUMaterial: baseColor %d metallicRoughness %d normal %d occlusion %d emissive %d\n",
        material.baseColor, material.metallicRoughness, material.normal, material.occlusion, material.emissive);
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
}
