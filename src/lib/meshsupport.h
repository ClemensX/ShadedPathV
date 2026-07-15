// support structures for mesh handling

#pragma once

// mesh and object flags, some are only useful for gltf file loading, some for object instances
enum class MeshFlags : int {
	MESH_TYPE_INVALID = 0,
	MESH_TYPE_PBR = 1,
	MESH_TYPE_SKINNED = 2,
	MESH_TYPE_NO_TEXTURES = 3,
	MESH_TYPE_FLIP_WINDING_ORDER = 4, // flip clockwise <-> counter-clockwise winding order
	MESH_TYPE_LOD = 5, // mesh contains LOD levels
	MESHLET_DEBUG_COLORS = 6, // apply vertex color to all triangles of one meshlet
	MESHLET_GENERATE = 7, // re-generate meshlet data if meshlet data file not found
    RENDER_TYPE_MOVING = 8, // object may change position and rotation
	MESH_TYPE_COUNT = -1 // always last
};

class MeshFlagsCollection {
private:
	std::bitset<32> flags;

public:
	MeshFlagsCollection() : flags(0) {}
	MeshFlagsCollection(MeshFlags flag) : flags(0) {
		setFlag(flag);
	}

	void setFlag(MeshFlags flag) {
		flags.set(static_cast<size_t>(flag));
	}

	void clearFlag(MeshFlags flag) {
		flags.reset(static_cast<size_t>(flag));
	}

	bool hasFlag(MeshFlags flag) const {
		return flags.test(static_cast<size_t>(flag));
	}
};

struct GPUCollectionIndex {
	uint32_t gpuCollectionInfoIndex; // index into CollectionInfos (== index of first major mesh of this collection)
	uint32_t mainMeshCount; // number of main meshes in this collection
};

struct GPUCollectionInfo {
	uint32_t collectionIndex; // 
	uint32_t meshNumberInCollection; // # mesh num inside collection
	uint32_t flags; // mesh flags, e.g. LOD
	uint32_t meshIndex; // index into gpuMeshInfos
	uint32_t next; // if > 0, index of next major mesh in collection, otherwise this is the last mesh of the collection
	uint32_t pad0;
};

struct GPUMeshInfo {
	uint64_t meshletOffset = 0; // offset into global mesh storage buffer
	uint64_t localIndexOffset = 0; // offset into global mesh storage buffer
	uint64_t globalIndexOffset = 0; // offset into global mesh storage buffer
	uint64_t vertexOffset = 0; // offset into global mesh storage buffer
	uint32_t meshletCount; // number of meshlets for this LOD
    uint32_t material; // during parsing: local material index, during GPU upload: global material index
	uint32_t index; // global mesh index
	uint32_t next; // next primitive (0 == no next primitive)
	BoundingBox boundingBox;
    const bool hasMeshlets() const {
        return meshletCount > 0 && vertexOffset > 0;
    }
};

// forward declaration
// MeshletsForMesh is a collection of meshlets for a single mesh
class MeshletsForMesh;

// structure for CPU-side metadata of meshes, not transferred to GPU
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
	bool boundingBoxAlreadySet = false;
}; 

struct GPUModel {
	glm::mat4 model;
	uint32_t flags;
	uint32_t meshNumber; // link to MeshInfo
	uint32_t material_lod_category;
	uint32_t materialIndex; // index into global material array
	//BoundingBox boundingBox; // probably not needed
};

// structure for CPU-side representation of objects, not transferred to GPU
struct SceneObject {
	glm::vec3 pos;
	glm::vec3 rot;
	glm::vec3 scale;
    int32_t index; // index into global model and object array
};

struct GPUMaterial {
	int32_t baseColor;
	int32_t metallicRoughness;
	int32_t normal;
	int32_t occlusion;
	int32_t emissive;

};

struct MeshFileEntry {
	std::string name;
    int32_t meshIndex;
};

struct MeshFile {
    std::string id;
    MeshFlagsCollection flags;
	std::vector<MeshFileEntry> meshes;
};