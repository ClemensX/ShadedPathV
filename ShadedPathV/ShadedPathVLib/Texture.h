class Util;

// forward declarations
struct MeshInfo;

struct TextureInfo
{
	string id;
	string filename;
	bool available = false; // true if this texture is ready for use in shader code
	ktxVulkanTexture vulkanTexture;
	VkImageView imageView = nullptr;
	// following gltf attributes are only valid during gltf parsing!!
	const float* gltfTexCoordData = nullptr;
	int gltfUVByteStride = 0;
};
typedef ::TextureInfo* TextureID;

// Texture Store:
class TextureStore {
public:
	// init texture store
	void init(ShadedPathEngine* engine);
	~TextureStore();
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(string filename, string id);
	::TextureInfo* getTexture(string id);
	// create texture id and slot for mesh texture with index
	::TextureInfo* createTextureSlot(MeshInfo* mesh, int index);
	// parse texture data during gltf parsing. create a KTX texture from memory and store into mesh
	void createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTex);
	// create textures ready to be used in shader code. Either normal textures (VK_IMAGE_TYPE_2D) of cube maps (VK_IMAGE_VIEW_TYPE_CUBE).
	// only ktx files are allowed with mipmaps already created.
	void createVulkanTextureFromKTKTexture(ktxTexture* ktxTexture, ::TextureInfo* textureInfo);
	void destroyKTXIntermediate(ktxTexture* ktxTex);

private:
	unordered_map<string, ::TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	Util* util;
	ktxVulkanDeviceInfo vdi;
};
