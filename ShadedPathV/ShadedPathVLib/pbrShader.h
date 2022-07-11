#pragma once

struct MeshInfo;
class WorldObject;

// line shader draws lines, it creates 2 pipelines, one for fixed lines (uploaded at start)
// and one for dynamic lines that change every frame
class PBRShader : public ShaderBase {
public:
	// We have to set max number of objects, as dynamic uniform buffers have to be allocated (one entry for each object in a large buffer)
	uint64_t MaxObjects = 1000;
	struct Vertex {
		glm::vec3 pos;
		//glm::vec3 normal;
		glm::vec2 uv0;
		//glm::vec2 uv1;
		//glm::vec4 joint0;
		//glm::vec4 weight0;
	};
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	struct DynamicUniformBufferObject {
		glm::mat4 model;
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
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
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

		return attributeDescriptions;
	}
	virtual ~PBRShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance

	// max # lines for dynamic adding for single frame
	// we limit this to allow for pre-allocated vertex buffer in thread ressources
	static const size_t MAX_DYNAMIC_LINES = 100000;

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

	// add lines - they will never  be removed
	void add(vector<LineDef>& linesToAdd);

	// get access to dynamic uniform buffer for an object
	DynamicUniformBufferObject* getAccessToModel(ThreadResources& tr, UINT num);
	
	// upload of all objects to GPU - only valid before first render
	void initialUpload();

	// per frame update of UBOs / MVPs
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
private:

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, WorldObject* obj, bool isRightEye = false);


	vector<LineDef> lines;
	int drawAddLinesSize;

	UniformBufferObject ubo, updatedUBO;
	bool disabled = false;
	// Inherited via Effect
	// set in init()

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	// create descriptor set layout (one per effect)
	virtual void createDescriptorSetLayout() override;
	// create descritor sets (one or more per render thread)
	virtual void createDescriptorSets(ThreadResources& res) override;

	// util methods
public:

	static void addCross(vector<LineDef>& lines, vec3 pos, vec4 color) {
		static float oDistance = 5.0f;
		LineDef crossLines[] = {
			// start, end, color
			{ glm::vec3(pos.x-oDistance, pos.y, pos.z), glm::vec3(pos.x+oDistance, pos.y, pos.z), color },
			{ glm::vec3(pos.x, pos.y-oDistance, pos.z), glm::vec3(pos.x, pos.y+oDistance, pos.z), color },
			{ glm::vec3(pos.x, pos.y, pos.z-oDistance), glm::vec3(pos.x, pos.y, pos.z+oDistance), color }
		};
		lines.insert(lines.end(), crossLines, crossLines + size(crossLines));
	}

	static void addZeroCross(vector<LineDef>& lines) {
		addCross(lines, vec3(), vec4(1.0f, 1.0f, 1.0f, 1.0f));
		static float oDistance = 5.0f;
		LineDef crossLines[] = {
			// start, end, color
			{ glm::vec3(-oDistance, 0.0f, 0.0f), glm::vec3(oDistance, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
			{ glm::vec3(0.0f, -oDistance, 0.0f), glm::vec3(0.0f, oDistance, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
			{ glm::vec3(0.0f, 0.0f, -oDistance), glm::vec3(0.0f, 0.0f, oDistance), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) }
		};
		lines.insert(lines.end(), crossLines, crossLines + size(crossLines));
	}
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