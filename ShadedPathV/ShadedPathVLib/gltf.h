// load gltf models via tinygltf.
// thread sync has to be done outside this class
// many details from https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/base/VulkanglTFModel.cpp

// forward declarations:
namespace tinygltf {
	class Model;
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
	void load(const unsigned char* data, int size, MeshCollection *mesh, std::string filename);
	// used for hook into tinygltf image loading:
	struct gltfUserData {
		ShadedPathEngine* engine = nullptr;
		MeshCollection* collection = nullptr;
	};
private:
	// load model from data pointer. Image data will also be parsed with results in MeshCollection->textureInfos[]
	void loadModel(tinygltf::Model& model, const unsigned char* data, int size, MeshCollection* coll, std::string filename);
	// copy model vertices and indices into vectors
	// index buffer will be 32 bit wide in all cases (VK_INDEX_TYPE_UINT32)
	void loadVertices(tinygltf::Model& model, MeshInfo* mesh, std::vector<PBRShader::Vertex>& verts, std::vector<uint32_t>& indexBuffer);
	// assign textures to their proper PBR members in mesh and read or create texture samplers
	void prepareTextures(tinygltf::Model& model, MeshCollection* mesh);
	// validate that gltf is within our parsable features
	void validateModel(tinygltf::Model& model, MeshCollection* mesh);
	ShadedPathEngine* engine = nullptr;

	VkSamplerAddressMode getVkWrapMode(int32_t wrapMode)
	{
		switch (wrapMode) {
		case 10497:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case 33071:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case 33648:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}
	}

	VkFilter getVkFilterMode(int32_t filterMode)
	{
		switch (filterMode) {
		case 9728:
			return VK_FILTER_NEAREST;
		case 9729:
			return VK_FILTER_LINEAR;
		case 9984:
			return VK_FILTER_NEAREST;
		case 9985:
			return VK_FILTER_NEAREST;
		case 9986:
			return VK_FILTER_LINEAR;
		case 9987:
			return VK_FILTER_LINEAR;
		}
	}
};