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
private:
	unordered_map<string, TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	Util* util;
};
