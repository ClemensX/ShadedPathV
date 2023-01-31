class Util;

// forward declarations
struct MeshInfo;

struct TextureInfo
{
	std::string id; // textures are usually access by their string name
	std::string filename;
	bool available = false; // true if this texture is ready for use in shader code
	ktxVulkanTexture vulkanTexture;
	VkImageView imageView = nullptr;
	// following gltf attributes are only valid during gltf parsing!!
	const float* gltfTexCoordData = nullptr;
	int gltfUVByteStride = 0;
	// usually our textures are created through ktx library, but there are some exceptions
	bool isKtxCreated = true;
	uint32_t index; // index used for shaders to access the right texture in the global texture array
};
typedef ::TextureInfo* TextureID;

// Texture Store. Textures have to be added during init phase, otherwise they will not be accessible by shaders
class TextureStore {
public:
	// max number of textures usable in engine is 1 million.
	// Do not confuse with maxTextures (actual max texture count) which is set by app at startup
	static const uint32_t UPPER_LIMIT_TEXTURE_COUNT = 1000000;
	// init texture store with max number of textures used
	void init(ShadedPathEngine* engine, size_t maxTextures);
	~TextureStore();
	// texture id for brdf lookup table:
	std::string BRDFLUT_TEXTURE_ID = "brdflut";
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(std::string filename, std::string id);
	::TextureInfo* getTexture(std::string id);
	// create texture slot for named texture
	::TextureInfo* createTextureSlot(std::string id);
	// create texture id and slot for mesh texture with index
	::TextureInfo* createTextureSlotForMesh(MeshInfo* mesh, int index);
	// parse texture data during gltf parsing. create a KTX texture from memory and store into mesh
	void createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTex);
	// create textures ready to be used in shader code. Either normal textures (VK_IMAGE_TYPE_2D) of cube maps (VK_IMAGE_VIEW_TYPE_CUBE).
	// only ktx files are allowed with mipmaps already created.
	void createVulkanTextureFromKTKTexture(ktxTexture* ktxTexture, ::TextureInfo* textureInfo);
	void destroyKTXIntermediate(ktxTexture* ktxTex);
	// Generate a BRDF integration map storing roughness/NdotV as a look-up-table
	// BRDF stands for Bidirectional Reflectance Distribution Function
	void generateBRDFLUT();
	// actual max texture count as set by app. This many descriptor entries will be allocated
	// trying to store more textures than this amount will create Error
	size_t getMaxSize() {
		return maxTextures;
	}
	// get const ref to map for easy and safe iteration:
	const std::unordered_map<std::string, ::TextureInfo>& getTexturesMap() { return textures; }

	VkDescriptorSetLayout layout = nullptr;
	VkDescriptorPool pool = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
private:
	std::unordered_map<std::string, ::TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	Util* util;
	ktxVulkanDeviceInfo vdi;
	size_t maxTextures = 0;
	// after adding a texture check that max size is not exceeded
	void checkStoreSize();
	// all creation methods have to call this internally:
	::TextureInfo* internalCreateTextureSlot(std::string id);
};
