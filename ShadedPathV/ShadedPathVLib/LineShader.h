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
		glm::mat4 wvp;
	};
	~LineShader();
	// one time effect initialization
	void init(ShadedPathEngine& engine);
	// thread resources initialization
	void initSingle(ThreadResources& tr);
	// add lines - they will never  be removed
	void add(vector<LineDef>& linesToAdd);
	// initial upload of all added lines - only valid before first render
	void initialUpload();
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
	VkDescriptorSetLayout descriptorSetLayout;
};
