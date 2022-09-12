#pragma once
// Billboards - draw simple billboards in world coordinates with direction and size
// Mesh Shader implemenation
struct BillboardDef {
	glm::vec3 pos;
	glm::vec3 dir;
	float w;
	float h;
	TextureID tex;
};

// per frame resources for this effect
struct BillboardFrameData {
public:
	std::vector<BillboardDef> addBillboards;
};

// Billboard shader draws billboards, it creates 2 pipelines, one for fixed (uploaded at start)
// and one for dynamic billoards that change every frame
class BillboardShader : public ShaderBase {
public:
	// max # billboards
	// we limit this to allow for pre-allocated vertex buffer in thread ressources
	static const size_t MAX_BILLBOARDS = 100000;
	// define billboard size and pos, --> to GPU as single UBO with all Billboards mem mapped
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 dir;
		float w;
		float h;
	};
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	// get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		// layout(location = 0) in vec3 inPosition;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// layout(location = 1) in vec3 direction;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, dir);
		// layout(location = 2) in float width;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 2;
		attributeDescriptions[1].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, w);
		// layout(location = 3) in float height;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 3;
		attributeDescriptions[1].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, h);

		return attributeDescriptions;
	}
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
	VkShaderModule vertShaderModule = nullptr;
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
	// vertex buffer for billboards
	VkBuffer billboardVertexBuffer = nullptr;
	// vertex buffer device memory
	VkDeviceMemory billboardVertexBufferMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
};
