#pragma once
// line effect - draw simple lines in world coordinates
struct LineDef {
	glm::vec3 start, end;
	glm::vec4 color;
};

// per frame resources for this effect
struct LineFrameData {
public:
	std::vector<LineDef> addLines;
};

// line shader draws lines, it creates 2 pipelines, one for fixed lines (uploaded at start)
// and one for dynamic lines that change every frame
class LineShader : public ShaderBase {
public:
	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::VertexBufferStatic },
		{ VulkanResourceType::IndexBufferStatic }
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec4 color;
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
		// layout(location = 1) in vec3 inColor;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
	virtual ~LineShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance

	// max # lines for dynamic adding for single frame
	// we limit this to allow for pre-allocated vertex buffer in thread ressources
	//static const size_t MAX_DYNAMIC_LINES = 100000;
	static const size_t MAX_DYNAMIC_LINES = 5000000;

	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
	virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
	virtual void createCommandBuffer(ThreadResources& tr) override;
	virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
	virtual void destroyThreadResources(ThreadResources& tr) override;

	// add lines - they will never  be removed
	void add(std::vector<LineDef>& linesToAdd);
	// global update: initiate update of global line data 
	// might take several frames until effect is visible: old buffer will be used until new one is ready
	void updateGlobal(std::vector<LineDef>& linesToAdd);
	// initial upload of all added lines - only valid before first render
	void initialUpload();

	// add lines for just one frame
	void addOneTime(std::vector<LineDef>& linesToAdd, ThreadResources& tr);

	// check if we need to switch resources for next render run
	void handleUpdatedResources(ThreadResources& tr);

	void createCommandBufferLineAdd(ThreadResources& tr);

	// clear line buffer, has to be called at begin of each frame
	// NOT after adding last group of lines
	void clearAddLines(ThreadResources& tr);

	// prepare command buffer for added lines
	void prepareAddLines(ThreadResources& tr);
	// per frame update of UBO / MVP
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO

	// resource switch after upload of new data has finished:
	void resourceSwitch(GlobalResourceSet set) override;
private:

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
	void recordDrawCommandAdd(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
	//// update cbuffer and vertex buffer
	//void update();
	//void updateUBO(UniformBufferObject newCBV);
	//// draw all lines in single call to GPU
	//void draw();
	//void destroy();


	std::vector<LineDef> lines;
	int drawAddLinesSize = 0;

	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;
	// Inherited via Effect
	// set in init()

	// vertex buffer for fixed lines (one buffer for all threads) 
	VkBuffer vertexBuffer = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemory = nullptr;
	// vertex buffer for updates: (one buffer for all threads) 
	VkBuffer vertexBufferUpdates = nullptr;
	// vertex buffer device memory for Updates
	VkDeviceMemory vertexBufferMemoryUpdates = nullptr;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	// create descriptor set layout (one per effect)
	virtual void createDescriptorSetLayout() override;
	// create descritor sets (one or more per render thread)
	virtual void createDescriptorSets(ThreadResources& res) override;
	// store line data on GPU, respect resource set
	void updateAndSwitch(std::vector<LineDef>* linesToAdd, GlobalResourceSet set);

	// util methods
public:

	static void addCross(std::vector<LineDef>& lines, glm::vec3 pos, glm::vec4 color) {
		static float oDistance = 5.0f;
		LineDef crossLines[] = {
			// start, end, color
			{ glm::vec3(pos.x-oDistance, pos.y, pos.z), glm::vec3(pos.x+oDistance, pos.y, pos.z), color },
			{ glm::vec3(pos.x, pos.y-oDistance, pos.z), glm::vec3(pos.x, pos.y+oDistance, pos.z), color },
			{ glm::vec3(pos.x, pos.y, pos.z-oDistance), glm::vec3(pos.x, pos.y, pos.z+oDistance), color }
		};
		lines.insert(lines.end(), crossLines, crossLines + std::size(crossLines));
	}

	static void addZeroCross(std::vector<LineDef>& lines) {
		addCross(lines, glm::vec3(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		static float oDistance = 5.0f;
		LineDef crossLines[] = {
			// start, end, color
			{ glm::vec3(-oDistance, 0.0f, 0.0f), glm::vec3(oDistance, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
			{ glm::vec3(0.0f, -oDistance, 0.0f), glm::vec3(0.0f, oDistance, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
			{ glm::vec3(0.0f, 0.0f, -oDistance), glm::vec3(0.0f, 0.0f, oDistance), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) }
		};
		lines.insert(lines.end(), crossLines, crossLines + std::size(crossLines));
	}

	private:
		struct LineShaderUpdateElement : ShaderUpdateElement {
			std::vector<LineDef>* linesToAdd;
		};
		std::array<LineShaderUpdateElement, 10> updateArray;
	protected:
		// global update method - guaranteed to be in sync mode: only 1 update at a time
		// but render threads may still use old data!
		void update(ShaderUpdateElement* el) override;
		size_t getUpdateArraySize() override {
			return updateArray.size();
		}
		// after global resource update each thread has to re-create command buffers and switch to new resource set
		void switchGlobalThreadResources(ThreadResources& res);
		ShaderUpdateElement* getUpdateElement(size_t i) override {
			return &updateArray[i];
		}

};

struct LineThreadResources : ShaderThreadResources {
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkFramebuffer framebufferAdd = nullptr;
	VkFramebuffer framebufferAdd2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkRenderPass renderPassAdd = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkPipeline graphicsPipelineAdd = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	VkCommandBuffer commandBufferAdd = nullptr;
	VkCommandBuffer commandBufferUpdate = nullptr;
	// vertex buffer for added lines
	VkBuffer vertexBufferAdd = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferAddMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
	std::vector<LineShader::Vertex> verticesAddLines;
};

// manage all resources associated with ONE line drawing resource
// we have: Global (fixed after init), Local (only valid for one draw), 1-2 Update (updated sometimes according to app needs)
class LineResourceManager {

};
