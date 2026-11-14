// CPU-side metadata for meshes - not transferred to GPU
// Depends on: PBRVertex (common_cpp_shader.h), PBRShader::PackedMeshletDesc (pbrShader.h), MeshletsForMesh (Object.h)
// Must be included after pbrShader.h and Object.h

#pragma once
// all info we need for lod handling, each MeshInfoMetadata links to this
struct LodMeshInfoMetadata {
    std::string basename;
    int lodLevels[10] = {-1};
};

struct MeshInfoMetadata {
    MeshInfoMetadata() {
    };
    std::string name;
    std::vector<PBRVertex> vertices;
    std::vector<uint32_t> indices;

    MeshletsForMesh meshletsForMesh;
    std::vector<uint32_t> meshletVertexIndices; // indices into vertices, used for meshlets
    // output: needed on GPU side
    std::vector<PBRShader::PackedMeshletDesc> outMeshletDesc;
    std::vector<uint8_t> outLocalIndexPrimitivesBuffer;   // local indices for primitives (3 indices per triangle)
    std::vector<uint32_t> outGlobalIndexBuffer; // vertex indices into vertex buffer
    uint32_t meshFileIndex; // link back to MeshFile vector
    bool boundingBoxAlreadySet = false;
    bool meshletStorageFileFound = false; // true if meshlet storage file was found and loaded
    const bool hasMeshlets() const {
        return meshletStorageFileFound || (meshletsForMesh.meshlets.size() > 0 && outMeshletDesc.size() > 0);
    }
    LodMeshInfoMetadata* lodInfo = nullptr; // pointer to LOD info for this mesh, if any
};
