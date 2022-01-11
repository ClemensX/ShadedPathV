class Util;
struct TextureInfo
{
	string id;
	string filename;
	// new apparoach: pre define a descriptor table for each texture, just set this with SetGraphicsRootDescriptorTable
	//D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable;
	bool available = false; // true if this texture is ready for use in shader code
	ktxVulkanTexture vulkanTexture;
	VkImageView imageView = nullptr;
};
typedef TextureInfo* TextureID;

// Texture Store:
class TextureStore {
public:
	// init d3d resources needed to initialize/upload textures later
	void init(ShadedPathEngine *engine);
	~TextureStore();
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(string filename, string id);
	TextureInfo *getTexture(string id);
private:
	unordered_map<string, TextureInfo> textures;
	ShadedPathEngine* engine = nullptr;
	//ComPtr<ID3D12CommandAllocator> commandAllocator;
	//ComPtr<ID3D12GraphicsCommandList> commandList;
	//ComPtr<ID3D12PipelineState> pipelineState;
	//ComPtr<ID3D12RootSignature> rootSignature;
	//FenceData updateFrameData;
	//DXGlobal* dxGlobal = nullptr;
	Util* util;
};