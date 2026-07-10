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
	// load all meshes from glTF file
	void loadMesh(std::string filename, std::string id, MeshFlagsCollection flags = MeshFlagsCollection());
    int32_t loadedMeshFileCount() const {
        return static_cast<int32_t>(meshFiles.size());
    }
    int32_t getMeshFileIndexByID(std::string id) const {
        auto it = meshFileIDs.find(id);
        if (it != meshFileIDs.end()) {
            int index = it->second;
            if (index >= 0 && index < meshFiles.size()) {
                return index;
            }
        }
        Error("MStore::getMeshFileIndexByID: Mesh file ID not found:");
        return -1; // keep compiler happy
    }
    const MeshFile* getMeshFileByID(std::string id) const {
                return &meshFiles[getMeshFileIndexByID(id)];
    }
    const GPUMeshInfo* getGPUMeshInfo(int32_t index) const;
    const GPUMaterial* getGPUMaterial(int32_t index) const;
    const MeshInfoMetadata* getMeshMetadata(int32_t index) const;

    // after parsing glTF file, this function will iterate through all meshes and materials and put them
    // into global buffers. All local indices will be converted to global indices.
    void addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<MeshInfoMetadata>& gpuMeshMetadata, const std::vector<GPUMaterial>& gpuMaterialInfos);
    // get the glTF parser instance
    glTF* getGLTF() { return &gltf; }

	glTF gltf;
private:
    size_t maxMeshes = 0; // maximum number of meshes that can be stored, set setLimits()
    std::optional<std::string> loadFile(std::string filename, std::vector<std::byte>& fileBuffer);

    // some flags may require additional work on the gltf base data, called from loadMesh()
    void handleFlags(GPUMeshInfo& meshInfo, MeshFlagsCollection flags);

    // handle gltf file info

    // add a mesh file ID to find MeshFile by name
    void addMeshFileID(std::string id, int32_t index) {
        meshFileIDs[id] = index;
    }
    std::vector<MeshFile> meshFiles; // list of loaded mesh files
    std::map<std::string, int32_t> meshFileIDs; // map from mesh file ID to index in meshFiles

    // maintain a list of mesh metadata for each loaded mesh, used for CPU-side operations
    std::vector<MeshInfoMetadata> meshMetadata;
    MeshInfoMetadata* getMeshMetadata(int32_t index);

};