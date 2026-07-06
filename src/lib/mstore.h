// new mesh store

#pragma once
struct MInfo {
	int index;
	int lodLevel; // LOD level of this mesh
	MeshFlagsCollection flags; // flags for this mesh
    bool available = false; // whether the mesh is available (loaded)
};

class MStore : public EngineParticipant {
public:
	void init();
    void setLimits(size_t maxMeshes) {
        this->maxMeshes = maxMeshes;
    }
	// load all meshes from glTF file, objects are referenced via id string according to this schema:
	// ref string == gltf mesh
	// =======================
	// id == mesh[0]
	// id.gltf_mesh_name == mesh with name == gltf_mesh_name
	// id.2 == mesh[2]
	void loadMesh(std::string filename, std::string id, MeshFlagsCollection flags = MeshFlagsCollection());
    // after parsing glTF file, this function will iterate through all meshes and materials and put them
    // into global buffers. All local indices will be converted to global indices.
    void addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<GPUMaterial>& gpuMaterialInfos);
    // get the glTF parser instance
    glTF* getGLTF() { return &gltf; }

	glTF gltf;
private:
    size_t maxMeshes = 0; // maximum number of meshes that can be stored, set setLimits()
	std::optional<std::string> loadFile(std::string filename, std::vector<std::byte>& fileBuffer);

};