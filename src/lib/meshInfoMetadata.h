// CPU-side metadata for meshes - not transferred to GPU
// Depends on: PBRVertex (common_cpp_shader.h), PBRShader::PackedMeshletDesc (pbrShader.h), MeshletsForMesh (Object.h)
// Must be included after pbrShader.h and Object.h

#pragma once

struct MeshInfoMetadata {
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
    const bool hasMeshlets() const {
        return meshletsForMesh.meshlets.size() > 0 && outMeshletDesc.size() > 0;
    }
};
