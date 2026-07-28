#pragma once

class Util;

// forward declarations
struct MeshInfo;

enum class TextureType : int {
	// default texture types
	TEXTURE_TYPE_MIPMAP_IMAGE = 0,
	TEXTURE_TYPE_HEIGHT = 1,
	TEXTURE_TYPE_GLTF = 2//,
	//TEXTURE_TYPE_DIFFUSE = 1,
	//TEXTURE_TYPE_SPECULAR = 2,
	//TEXTURE_TYPE_NORMAL = 3,
	//TEXTURE_TYPE_AMBIENT_OCCLUSION = 5,
	//TEXTURE_TYPE_EMISSIVE = 6,
	//TEXTURE_TYPE_BRDF_LUT = 7,
	//TEXTURE_TYPE_CUBEMAP = 8,
	//TEXTURE_TYPE_IRRADIANCE = 9,
	//TEXTURE_TYPE_PREFILTER = 10,
	//TEXTURE_TYPE_LUT = 11,
	//TEXTURE_TYPE_COUNT = 12 // always last, to be used as array size
};

// Texture flags, used to set special properties for textures
// values can be or-ed together, always use hasFlag() to check if a flag is set
enum class TextureFlags : unsigned int {
	NONE = 0,
	// keep buffer to access heightmap from CPU
	KEEP_DATA_BUFFER = 1 << 0, // 1
    // first float in height map is for point(xmax, zmax), 2nd for point(xmax-1, zmax), and so on
    ORIENTATION_RAW_START_WITH_XMAX_ZMAX = 1 << 1, // 2
//	REPEAT = 1 << 1,  // 2
//	MIRROR = 1 << 2   // 4
};

// Enable bitwise operations on the enum class
inline TextureFlags operator|(TextureFlags a, TextureFlags b) {
	return static_cast<TextureFlags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline TextureFlags operator&(TextureFlags a, TextureFlags b) {
	return static_cast<TextureFlags>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline TextureFlags& operator|=(TextureFlags& a, TextureFlags b) {
	return a = a | b;
}

inline TextureFlags& operator&=(TextureFlags& a, TextureFlags b) {
	return a = a & b;
}

// Check if a flag is set
inline bool hasFlag(TextureFlags value, TextureFlags flag) {
	return (value & flag) == flag;
}

struct TextureInfo
{
	std::string id; // textures are usually access by their string name
	std::string filename;
    const bool isAvailable() const {
        return available;
    }
	ktxVulkanTexture vulkanTexture = {};
	VkImageView imageView = nullptr;
	// following gltf attributes are only valid during gltf parsing!!
	const float* gltfTexCoordData = nullptr;
	int gltfUVByteStride = 0;
	// usually our textures are created through ktx library, but there are some exceptions
	bool isKtxCreated = true;
	uint32_t index = 0; // index used for shaders to access the right texture in the global texture array
	TextureType type = TextureType::TEXTURE_TYPE_MIPMAP_IMAGE;
    VkSampler sampler = nullptr; // for texture type gltf we use the sampler directly
	std::vector<float> float_buffer;
    TextureFlags flags = TextureFlags::NONE;
    bool hasFlag(TextureFlags flag) const {
        return ::hasFlag(flags, flag);
    }
    size_t hash = 0; // generated from raw image data, used for texture reuse and validation
    bool textureIsReused = false; // true if this texture info was reused from another texture with same hash value
private:
	bool available = false; // only set by TextureStore
    friend class TextureStore;

};
typedef ::TextureInfo* TextureID;

// Texture Store. Textures have to be added during init phase, otherwise they will not be accessible by shaders
class TextureStore {
public:
	// max number of textures usable in engine is 1 million.
	// Do not confuse with maxTextures (actual max texture count) which is set by app at startup
#   if defined(__APPLE__)
	static const uint32_t UPPER_LIMIT_TEXTURE_COUNT = 640;
#   elif defined(USE_SMALL_GRAPHICS)
	static const uint32_t UPPER_LIMIT_TEXTURE_COUNT = 1800;
#   else
	static const uint32_t UPPER_LIMIT_TEXTURE_COUNT = 1000000;
#   endif
	// init texture store with max number of textures used
	void init(ShadedPathEngine* engine, size_t maxTextures);
	~TextureStore();
	// texture id for brdf lookup table:
	std::string BRDFLUT_TEXTURE_ID = "brdflut";
	// texture id for brdf lookup table:
	std::string IRRADIANCE_TEXTURE_ID = "irradiance";
	// texture id for brdf lookup table:
	std::string PREFILTEREDENV_TEXTURE_ID = "prefilteredenv";
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(std::string filename, std::string id, TextureType type = TextureType::TEXTURE_TYPE_MIPMAP_IMAGE, TextureFlags flags = TextureFlags::NONE);
	// count currently avilable textures
    int size() {
        return static_cast<int>(textures.size());
    }
	// get texture by name
	::TextureInfo* getTexture(std::string id);
	// get texture by index to global descriptor table (== index)
	::TextureInfo* getTextureByIndex(uint32_t index);
	// create texture slot for named texture
	::TextureInfo* createTextureSlot(std::string id);
	// create texture id and slot for mesh texture with index
	::TextureInfo* createTextureSlotForMesh(MeshInfo* mesh, int index);
	// parse texture data during gltf parsing. create a KTX texture from memory and store into mesh
	void createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTex);
	// create textures ready to be used in shader code. Either normal textures (VK_IMAGE_TYPE_2D) of cube maps (VK_IMAGE_VIEW_TYPE_CUBE).
	// only ktx files are allowed with mipmaps already created.
	void createVulkanTextureFromKTKTexture(ktxTexture* ktxTexture, ::TextureInfo* textureInfo);
    // get access to first level 0 layer 0 of image data (== first mipmap level, first layer, first face)
	// used either for image data storage of hash generation.
    void getAccessToImageDataFromKTX(ktxTexture* ktxTexture, size_t &size, void ** data);
	void destroyKTXIntermediate(ktxTexture* ktxTex);
	// Generate a BRDF integration map storing roughness/NdotV as a look-up-table
	// BRDF stands for Bidirectional Reflectance Distribution Function
	void generateBRDFLUT();
	void generateCubemaps(std::string skyboxTexture, int32_t dimIrradiance = 64, VkFormat formatIrradiance = VK_FORMAT_R32G32B32A32_SFLOAT, int32_t dimPrefilteredEnv = 512, VkFormat formatPrefilteredEnv = VK_FORMAT_R16G16B16A16_SFLOAT);
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
	// activate / deactivate texture
    void setTextureActive(std::string id, bool active);
	// validation: was hash generated?
	void validateTexture(TextureInfo* ti);
	// generate hash value from raw image data
	size_t generateHash(const unsigned char* bytes, size_t size); 
    // get texture by hash value, used for texture reuse and validation,
    // simply iterates through textures and compares hash values, inefficient, but even for 1000s of textures should be fairly quickly
	TextureInfo* getTextureByHash(size_t hash);
    // we may reuse a texture with same hash value, we have to make sure that the sampler is the same, otherwise we have a problem with different samplers for same texture data
    // this method will throw an error if the samplers are not the same. For eas of use an index < 0 can be passed, meaning texture unavailable
	// (happens when a specific texture type is not available in a gltf model)
	void setAndCheckSampler(int32_t index, VkSampler sampler) {
        if (index < 0) return; // texture not available, nothing to check
        TextureInfo* ti = getTextureByIndex(index);
		//if (ti->sampler != nullptr && ti->textureIsReused == false) {
		//	Error("setAndCheckSampler: internal error texture index " + std::to_string(index) + " was tried to reuse but flag was not set");
		//}
		if (ti->sampler != nullptr && ti->sampler != sampler) {
			Error("setAndCheckSampler: texture index " + std::to_string(index) + " has different sampler than reused texture");
		}
		if (ti->sampler == nullptr) {
			ti->sampler = sampler;
        }
	}
private:
	std::unordered_map<std::string, ::TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	Util* util = nullptr;
	ktxVulkanDeviceInfo vdi = {};
	size_t maxTextures = 0;
	// after adding a texture check that max size is not exceeded
	void checkStoreSize();
	// all creation methods have to call this internally:
	::TextureInfo* internalCreateTextureSlot(std::string id);
};

// vertex def for cubemaps calculation
struct vkglTF_Model_Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv0;
	glm::vec2 uv1;
	glm::uvec4 joint0;
	glm::vec4 weight0;
	glm::vec4 color;
};
