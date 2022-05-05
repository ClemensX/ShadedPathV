#pragma once
// line effect - draw simple lines in world coordinates
struct LineDef {
	glm::vec3 start, end;
	glm::vec4 color;
};

// per frame resources for this effect
struct LineFrameData {
public:
	vector<LineDef> addLines;
};

// line shader draws lines, it creates 2 pipelines, one for fixed lines (uploaded at start)
// and one for dynamic lines that change every frame
class LineShader : public ShaderBase {
public:
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
	static const size_t MAX_DYNAMIC_LINES = 100000;

	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
	virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
	virtual void createCommandBuffer(ThreadResources& tr) override;
	virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
	// add lines - they will never  be removed
	void add(vector<LineDef>& linesToAdd);
	// initial upload of all added lines - only valid before first render
	void initialUpload();

	// add lines for just one frame
	void addOneTime(vector<LineDef>& linesToAdd, ThreadResources& tr);

	void createCommandBufferLineAdd(ThreadResources& tr);

	// clear line buffer, has to be called at begin of each frame
	// NOT after adding last group of lines
	void clearAddLines(ThreadResources& tr);

	// prepare command buffer for added lines
	void prepareAddLines(ThreadResources& tr);
	// per frame update of UBO / MVP
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo);
private:

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer);
	void recordDrawCommandAdd(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer);
	//// update cbuffer and vertex buffer
	//void update();
	//void updateUBO(UniformBufferObject newCBV);
	//// draw all lines in single call to GPU
	//void draw();
	//void destroy();


	vector<LineDef> lines;
	int drawAddLinesSize;

	UniformBufferObject ubo, updatedUBO;
	bool disabled = false;
	// Inherited via Effect
	// set in init()

	// vertex buffer for fixed lines (one buffer for all threads) 
	VkBuffer vertexBuffer = nullptr;
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
	static void addZeroCross(vector<LineDef>& lines) {
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
