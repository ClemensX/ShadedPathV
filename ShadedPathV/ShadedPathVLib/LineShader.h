#pragma once
// line effect - draw simple lines in world coordinates
struct LineDef {
	glm::vec3 start, end;
	glm::vec4 color;
};

class LineShaderData {
public:
	vector<LineDef> lines;
	vector<LineDef> oneTimeLines;
	~LineShaderData();
	UINT numVericesToDraw = 0;
};

// per frame resources for this effect
struct LineFrameData {
public:
	//friend class DXGlobal;
};

class LineShader {
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
	~LineShader();
	// one time effect initialization
	void init(ShadedPathEngine& engine);
	// thread resources initialization
	void initSingle(ThreadResources& tr);
	// add lines - they will never  be removed
	void add(vector<LineDef>& linesToAdd);
	// initial upload of all added lines - only valid before first render
	void initialUpload();

	void createRenderPass(ThreadResources& tr);
	void createCommandBufferLine(ThreadResources& tr);

	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer);
	// add lines just for next draw call
	void addOneTime(vector<LineDef>& linesToAdd, unsigned long& user);
	// update cbuffer and vertex buffer
	void update();
	void updateUBO(UniformBufferObject newCBV);
	// draw all lines in single call to GPU
	void draw();
	void destroy();

private:
	vector<LineDef> lines;
	//vector<LineDef> addLines;
	bool dirty;
	int drawAddLinesSize;

	//ComPtr<ID3D12PipelineState> pipelineState;
	//ComPtr<ID3D12RootSignature> rootSignature;
	//void preDraw(int eyeNum);
	//void postDraw();
	UniformBufferObject ubo, updatedUBO;
	//bool signalUpdateCBV = false;
	//mutex mutex_lines;
	//void drawInternal(int eyeNum = 0);
	//void updateTask();
	//UINT numVericesToDraw = 0;
	LineFrameData appDataSets[2];
	bool disabled = false;
	// Inherited via Effect
	// set in init()
	VkDevice device = nullptr;
	GlobalRendering* global = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	// create descriptor set layout (one per effect)
	void createDescriptorSetLayout();
	// create uniform buffers (one per render thread)
	void createUniformBuffer(ThreadResources& res);
	// create descritor sets (one or more per render thread)
	void createDescriptorSets(ThreadResources& res);
	void createDescriptorPool(ThreadResources& res);
	VkDescriptorSetLayout descriptorSetLayout;
};
