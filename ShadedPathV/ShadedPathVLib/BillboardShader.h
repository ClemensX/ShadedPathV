#pragma once
// Billboards - draw simple billboards in world coordinates with direction and size
// Mesh Shader implemenation
struct BillboardDef {
	vec3 pos;
	vec3 dir;
	float w;
	float h;
	TextureID tex;
};

// per frame resources for this effect
struct BillboardFrameData {
public:
	vector<BillboardDef> addBillboards;
};

// Billboard shader draws billboards, it creates 2 pipelines, one for fixed (uploaded at start)
// and one for dynamic billoards that change every frame
class BillboardShader : public ShaderBase {
public:
	// max # billboards
	// we limit this to allow for pre-allocated vertex buffer in thread ressources
	static const size_t MAX_BILLBOARDS = 100000;
	// define billboard size and pos, --> to GPU as single UBO with all Billboards mem mapped
	struct BBoard {
		vec3 pos;
		vec3 dir;
		float w;
		float h;
	};
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	virtual ~BillboardShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
	virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
	virtual void createCommandBuffer(ThreadResources& tr) override;
	virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
	virtual void destroyThreadResources(ThreadResources& tr) override;

	// initial upload of all added lines - only valid before first render
	void initialUpload();

	// per frame update of UBO / MVP
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
private:

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);

	UniformBufferObject ubo, updatedUBO;
	bool disabled = false;

	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkShaderModule meshShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	// create descriptor set layout (one per effect)
	virtual void createDescriptorSetLayout() override;
	// create descritor sets (one or more per render thread)
	virtual void createDescriptorSets(ThreadResources& res) override;

	// util methods
public:
};

struct BillboardThreadResources : ShaderThreadResources {
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	// billboard buffer device memory
	VkDeviceMemory billboardBufferMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
};
