class Util;
struct TextureInfo
{
	string id;
	string filename;
	bool available = false; // true if this texture is ready for use in shader code
	ktxVulkanTexture vulkanTexture;
	VkImageView imageView = nullptr;
};
typedef TextureInfo* TextureID;

// Texture Store:
class TextureStore {
public:
	// init texture store
	void init(ShadedPathEngine* engine);
	~TextureStore();
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(string filename, string id);
	TextureInfo* getTexture(string id);
	// create texture id and slot for mesh texture with index
	TextureInfo* createTextureSlot(MeshInfo* mesh, int index);
	// parse texture data during gltf parsing. create a KTX texture from memory and store into mesh
	void createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTex);
	void createVulkanTextureFromKTKTexture(ktxTexture* ktxTexture, TextureInfo* textureInfo);
	void destroyKTXIntermediate(ktxTexture* ktxTex);

private:
	unordered_map<string, TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	Util* util;
	ktxVulkanDeviceInfo vdi;
};
