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

// forward
class PBRSubShader;

// pbr shader draws objects read from glTF files with PBR lighing
class PBRShader : public ShaderBase {
public:
	// We have to set max number of objects, as dynamic uniform buffers have to be allocated (one entry for each object in a large buffer)
	uint64_t MaxObjects = 1000;

	std::vector<PBRSubShader> globalSubShaders;

	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::UniformBufferDynamic },
		{ VulkanResourceType::GlobalTextureSet },
		{ VulkanResourceType::VertexBufferStatic }
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv0;
		glm::vec2 uv1;
		glm::uvec4 joint0;
		glm::vec4 weight0;
		glm::vec4 color;
	};
	struct shaderValuesParams {
		glm::vec4 lightDir;
		float exposure = 4.5f;
		float gamma = 2.2f;
		float prefilteredCubeMipLevels;
		float scaleIBLAmbient = 1.0f;
		float debugViewInputs = 0;
		float debugViewEquation = 0;
		float pad0;
		float pad1;
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 baseColor = glm::vec4(1.0f);
		glm::vec3 camPos = glm::vec3(1.0f);
	};

	// MUST match shader definition: pbr.vert, pbr.frag
    // DO NOT USE arrays for padding on glsl side! only single variables like uint pad0, uint pad1, ...
#define MAX_NUM_JOINTS 128
	struct PBRTextureIndexes {
		uint32_t baseColor; // uint in shader
		uint32_t metallicRoughness; // uint in shader
		uint32_t normal; // uint in shader
		uint32_t occlusion; // uint in shader
		uint32_t emissive; // uint in shader
		uint32_t pad0[3]; // 12 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!
	};
	struct ShaderMaterial {
		glm::vec4 baseColorFactor;
		glm::vec4 emissiveFactor;
		glm::vec4 diffuseFactor;
		glm::vec4 specularFactor;
		float workflow;
		int baseColorTextureSet;
		int physicalDescriptorTextureSet;
		int normalTextureSet;
		int occlusionTextureSet;
		int emissiveTextureSet;
		float metallicFactor;
		float roughnessFactor;
		float alphaMask;
		float alphaMaskCutoff;
		float emissiveStrength;
	};
    // the dynamic uniform buffer is peramnently mapped to CPU memory for fast updates
	struct alignas(16) DynamicModelUBO {
		glm::mat4 model; // 16-byte aligned
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]; // 16-byte aligned
		uint32_t jointcount; // 4-byte aligned
		uint32_t pad0[3]; // 12 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!
		PBRTextureIndexes indexes; // 4-byte aligned
        shaderValuesParams params; // 16-byte aligned
        ShaderMaterial material; // 16-byte aligned
	};
	// Array entries of DynamicModelUBO have to respect hardware alignment rules
	uint64_t alignedDynamicUniformBufferSize = 0;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	// get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
	static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};

		// layout(location = 0) in vec3 inPos;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		// layout(location = 1) in vec3 inNormal;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		// layout(location = 2) in vec2 inUV0;
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, uv0);

		// layout(location = 3) in vec2 inUV1;
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, uv1);

		// layout(location = 4) in uvec4 inJoint0;
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[4].offset = offsetof(Vertex, joint0);

		// layout(location = 5) in vec4 inWeight0;
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(Vertex, weight0);

		// layout(location = 6) in vec4 inColor0;
		attributeDescriptions[6].binding = 0;
		attributeDescriptions[6].location = 6;
		attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[6].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	virtual ~PBRShader() override;

	// shader initialization, end result is a shader sub resource for each worker thread
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// frame resources initialization
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
	// create command buffers. One time auto called before rendering starts
	// we create one command buffer for every mesh loaded
	virtual void createCommandBuffer(FrameResources& tr) override;
	// add the pre-computed command buffer for the current object
	virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;

	// get access to dynamic uniform buffer for an object
	DynamicModelUBO* getAccessToModel(FrameResources& tr, UINT num);
	
	// upload of all objects to GPU - only valid before first render
	void initialUpload();

	// per frame update of UBOs / MVPs
	void uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
	// one-time prefill PBR parameters in the dynamic Uniform Buffer.
    // Called once before rendering starts. Apps can change settings anytime by accessing the dynamic buffer via getAccessToModel()
	void prefillModelParameters(FrameResources& tr);

	void fillTextureIndexesFromMesh(PBRTextureIndexes& ind, MeshInfo* mesh);

private:
	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
};

/*
 * PBRSubShader includes everything for one shader invocation.
 * Currently only 1 sub shader
 */
class PBRSubShader {
public:
	// name is used in shader debugging
	void init(PBRShader* parent, std::string debugName);
	void setVertShaderModule(VkShaderModule sm) {
		vertShaderModule = sm;
	}
	void setFragShaderModule(VkShaderModule sm) {
		fragShaderModule = sm;
	}
	void initSingle(FrameResources& tr, ShaderState& shaderState);
	void setVulkanResources(VulkanResources* vr) {
		vulkanResources = vr;
	}

	// All sections need: buffer allocation and recording draw commands.
	// Stage they are called at will be very different
	void allocateCommandBuffer(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName);
	void addRenderPassAndDrawCommands(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer);

	void createGlobalCommandBufferAndRenderPass(FrameResources& tr);
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, WorldObject* obj, bool isRightEye = false);
	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, PBRShader::UniformBufferObject& ubo, PBRShader::UniformBufferObject& ubo2);

	void destroy();

	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
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
	VkPipelineLayout pipelineLayout = nullptr;

private:
	PBRShader* pbrShader = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	FrameResources* frameResources = nullptr;
};
