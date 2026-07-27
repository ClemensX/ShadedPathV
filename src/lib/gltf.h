// load gltf models via tinygltf.
// thread sync has to be done outside this class
// many details from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/base/VulkanglTFModel.cpp

// forward declarations:
namespace tinygltf {
	class Model;
	struct Sampler;
    struct Material;
}
struct MeshInfo;
struct MeshCollection;

class glTF {
public:
	inline static const std::string BASE_COLOR_TEXTURE = "baseColorTexture";

	void init(ShadedPathEngine* e);
	// only load vertex and index info from model. Useful for wireframe rendering
	void loadVertices(const unsigned char* data, int size, MeshInfo* mesh, std::vector<PBRShader::Vertex>& verts, std::vector<uint32_t>& indexBuffer, std::string filename);
	// load model and prepare for PBR rendering
	void load(const unsigned char* data, int size, MeshCollection* mesh, std::string filename);
	// load model and prepare for PBR rendering
	void load2(const unsigned char* data, int size, std::string filename);
	// used for hook into tinygltf image loading:
	struct gltfUserData {
		ShadedPathEngine* engine = nullptr;
		MeshCollection* collection = nullptr;
	};
	void mapTinyGLTFSamplerToVulkan(const tinygltf::Sampler& gltfSampler, VkSamplerCreateInfo& vkSamplerInfo);
	void initTextureMap() {
		indexMap.clear();
	}
	void mapFileTextureIndexToGlobalTextureArray(int image_idx, int global_idx) {
		indexMap[image_idx] = global_idx;
	}
	size_t getTextureCount() const {
		return indexMap.size();
	}
	size_t getGlobalTextureIndex(int image_idx) const {
		auto it = indexMap.find(image_idx);
		if (it != indexMap.end()) {
			return it->second;
		}
		return static_cast<size_t>(-1); // or some other invalid value
	}
private:
	// load model from data pointer. Image data will also be parsed with results in MeshCollection->textureInfos[]
	void loadModel(tinygltf::Model& model, const unsigned char* data, int size, MeshCollection* coll, std::string filename);
	// new load model from data pointer. Image data will also be parsed with results in global texture store
	void loadModel2(tinygltf::Model& model, const unsigned char* data, int size, std::string filename);
	// copy model vertices and indices into vectors
    // index buffer will be 32 bit wide in all cases (VK_INDEX_TYPE_UINT32)
    // now supports selecting a specific primitive within a glTF mesh
    void loadVertices(tinygltf::Model& model, MeshInfo* mesh, std::vector<PBRShader::Vertex>& verts, std::vector<uint32_t>& indexBuffer, int gltfMeshIndex, int primitiveIndex);
	// load vertices core:
    void loadVerticesCore(tinygltf::Model& model, std::vector<PBRShader::Vertex>& verts, std::vector<uint32_t>& indexBuffer, int gltfMeshIndex, int primitiveIndex);
    // assign textures to their proper PBR members in mesh and read or create texture samplers
    // now supports selecting a specific primitive within a glTF mesh
    void prepareTexturesAndMaterials(tinygltf::Model& model, MeshCollection* coll, int gltfMeshIndex, int primitiveIndex, MeshInfo* mesh);
	// validate that gltf is within our parsable features
	void validateModel(tinygltf::Model& model, MeshCollection* mesh);
	// collect scale and rotation info from gltf nodes hierarchy and store in MeshInfo
	void collectBaseTransform(tinygltf::Model& model, MeshInfo *mesh);
	ShadedPathEngine* engine = nullptr;

	// map local texture index to global texture array index:
	std::map<int, int> indexMap;

	// after basic glTF parsing, this function will parse all meshes and store them in the mesh store
	void parseGltfModel(tinygltf::Model& model);

	inline bool IsMetallicRoughnessWorkflow(const tinygltf::Material& mat);
};