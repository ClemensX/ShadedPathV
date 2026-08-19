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
    // load all meshes from glTF file. If already loaded, return pointer to existing MeshFile.
    MeshFile* loadMesh(std::string filename, std::string id, MeshFlagsCollection flags = MeshFlagsCollection());
    // don't use id string - will auto generate one from MeshFile index. If already loaded, return pointer to existing MeshFile.
    MeshFile* loadMesh(std::string filename, MeshFlagsCollection flags = MeshFlagsCollection());
    // check pre-existing id or filename, return pointer to MeshFile if found, nullptr if not found 
    MeshFile* checkMeshFile(std::string filename, std::string);

    int32_t loadedMeshFileCount() const {
        return static_cast<int32_t>(meshFiles.size());
    }

    std::vector<MeshFile> getMeshFiles() const {
        return meshFiles;
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
    MeshFile* getMeshFileByID(std::string id) {
                return &meshFiles[getMeshFileIndexByID(id)];
    }
    GPUMeshInfo* getGPUMeshInfo(int32_t index) ;
    MeshInfoMetadata* getMeshMetadata(int32_t index);
    GPUModel* getGPUModel(int32_t index);
    int getUsedModelCount() const;
    GPUModel* getGPUMovingModel(int32_t index);
    SceneObject* getSceneObject(int32_t index);
    SceneObject* getMovingSceneObject(int32_t index);
    GPUMaterial* getGPUMaterial(int32_t index);
    GPUFrameParam* getGPUFrameParam(int32_t index);
    // upload all meshes during init phase, called from PBRShader
    void uploadAllMeshes();

    // after parsing glTF file, this function will iterate through all meshes and materials and put them
    // into global buffers. All local indices will be converted to global indices.
    void addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<MeshInfoMetadata>& gpuMeshMetadata,
                            const std::vector<GPUMaterial>& gpuMaterialInfos, const std::vector<MaterialMetadata>& gpuMaterialMetadata);
    // get the glTF parser instance
    glTF* getGLTF() { return &gltf; }

	glTF gltf;

    // to render an object using meshlets we need:
    // 1. meshlet desc buffer, most important: get global index start for each meshlet
    // 2. global index buffer, which contains indices into the global vertex buffer
    // 3. local index buffer, byte buffer which maps local meshlet vertex index to global index buffer: byte val + global index start is the index where the actual vertex is found
    void calculateMeshlets(GPUMeshInfo* m, uint32_t meshlet_flags, uint32_t vertexLimit = GLEXT_MESHLET_VERTEX_COUNT, uint32_t primitiveLimit = GLEXT_MESHLET_PRIMITIVE_COUNT);

    // objects

    // add a new object to the scene, returns pointer to SceneObject. The mesh_index is the index of the mesh in the global mesh buffer.
    // for moving objects, the MeshFlagsCollection should have the RENDER_TYPE_MOVING flag set.
    SceneObject* addObject(int32_t mesh_index, glm::vec3 pos, MeshFlagsCollection flags = MeshFlagsCollection());

    // get the full list of stationary objects:
    std::vector<int32_t> getStationaryObjects() {
        return stationaryObjectIndices;
    }

    // Meshlets

    // write meshlet data for all meshes in the collection to file, return true if successful
    bool writeMeshletStorageFile(MeshFile* mfile);
    // load meshlet data for all meshes of a collection from file, return true if successful, error if #items and #meshlet data sets do not match
    bool loadMeshletStorageFile(MeshFile* mfile);

    // Util methods

    // return the mesh bounding box from raw mesh data. No transforms applied. Will never change after initial calculation.
    void getBoundingBox(BoundingBox& box, GPUMeshInfo& meshInfo);
    bool checkBoundingBoxPlausibility(int32_t meshIndex);
    void logVertex(const PBRShader::Vertex& v);
    void logTriangleFromGlTF(int num, GPUMeshInfo* mesh);
    // apply fixed colors to all vertices of one meshlet (useful for debugging)
    // may not be totally correct if some vertices are shared between meshlets (color value will be overwritten)
    void applyDebugMeshletColorsToVertices(GPUMeshInfo* mesh);
    // apply same color to all triangles of the meshlets (useful for debugging)
    // this simply marks the meshlet with a debug flag, the actual color is applied in the shader
    void applyDebugMeshletColorsToMeshlets(GPUMeshInfo* mesh);
    void logMeshletStats(GPUMeshInfo* mesh);

private:
    size_t maxMeshes = 0; // maximum number of meshes that can be stored, set setLimits()
    std::optional<std::string> loadFile(std::string filename, std::vector<std::byte>& fileBuffer);

    // some flags may require additional work on the gltf base data, called from loadMesh()
    void handleFlags(GPUMeshInfo& meshInfo, MeshFlagsCollection flags);

    // generate or load meshlet data. will show error log message if meshlet file not found and regenerate == false
    void aquireMeshletData(MeshFile* mfile, bool regenerateMeshletData = false);

    void uploadMesh(GPUMeshInfo* mi);
    // handle gltf file info

    void checkVertexDuplication(GPUMeshInfo* mesh);
    // add a mesh file ID to find MeshFile by name
    void addMeshFileID(std::string id, int32_t index) {
        meshFileIDs[id] = index;
    }
    std::vector<MeshFile> meshFiles; // list of loaded mesh files
    std::map<std::string, int32_t> meshFileIDs; // map from mesh file ID to index in meshFiles

    // maintain a list of mesh metadata for each loaded mesh, used for CPU-side operations
    std::vector<MeshInfoMetadata> meshMetadata;

    // maintain a list of scene objects. Used for CPU-side of GPUModels. These are GPU buffers. always allocated with their max entries
    std::vector<SceneObject> sceneObjects;
    std::vector<SceneObject> movingSceneObjects;

    std::vector<int32_t> stationaryObjectIndices;
    // no checks, directly access cpp buffer
    GPUMeshInfo* getGPUMeshInfoInternal(int32_t index);
};