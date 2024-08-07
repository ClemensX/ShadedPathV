#pragma once

struct MeshInfo;
class WorldObject;

// make sure to match the push_constant layout in the shader
struct pbrPushConstants {
	unsigned int mode; // 0: standard pbr metallicRoughness, 1: pre-light vertices with color in vertex structure
};

const VkPushConstantRange pbrPushConstantRange = {
	VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
	0, // offset
	sizeof(pbrPushConstants) // size
};

// pbr shader draws objects read from glTF files with PBR lighing
class PBRShader : public ShaderBase {
public:
	// We have to set max number of objects, as dynamic uniform buffers have to be allocated (one entry for each object in a large buffer)
	uint64_t MaxObjects = 1000;

	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::UniformBufferDynamic },
		{ VulkanResourceType::GlobalTextureSet },
		{ VulkanResourceType::VertexBufferStatic }
	};

	struct Vertex {
		glm::vec3 pos;
		//glm::vec3 normal;
		glm::vec2 uv0;
		glm::vec4 color;
		//glm::vec2 uv1;
		//glm::vec4 joint0;
		//glm::vec4 weight0;
	};
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	// MUST match shader definition: pbr.vert, pbr.frag
	struct PBRTextureIndexes {
		unsigned int baseColor; // uint in shader
	};
	struct DynamicUniformBufferObject {
		glm::mat4 model;
		PBRTextureIndexes indexes;
	};
	// Array entries of DynamicUniformBufferObject have to respect hardware alignment rules
	uint64_t alignedDynamicUniformBufferSize = 0;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	// get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		// layout(location = 0) in vec3 inPosition;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// layout(location = 1) in vec3 inColor;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, uv0);
		// layout(location = 2) in vec4 inColor0;
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, color);


		return attributeDescriptions;
	}
	virtual ~PBRShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance

	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
	virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
	// create command buffers. One time auto called before rendering starts
	// we create one command buffer for every mesh loaded
	virtual void createCommandBuffer(ThreadResources& tr) override;
	// add the pre-computed command buffer for the current object
	virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
	virtual void destroyThreadResources(ThreadResources& tr) override;

	// get access to dynamic uniform buffer for an object
	DynamicUniformBufferObject* getAccessToModel(ThreadResources& tr, UINT num);
	
	// upload of all objects to GPU - only valid before first render
	void initialUpload();

	// per frame update of UBOs / MVPs
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
private:

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, WorldObject* obj, bool isRightEye = false);

	// preset PBR texture indexes in the dynamic Uniform Buffer.
	// Application code can overwrite the setting in drawFrame()
	void prefillTextureIndexes(ThreadResources& tr);

	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;

	// util methods
public:
};

struct PBRThreadResources : ShaderThreadResources {
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	// VP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// Model buffers
	VkBuffer dynamicUniformBuffer = nullptr;
	// VP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
	// M buffer device memory
	VkDeviceMemory dynamicUniformBufferMemory = nullptr;
	void* dynamicUniformBufferCPUMemory = nullptr;
};