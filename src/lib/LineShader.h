#pragma once

/*
 * Line Shader - draw various lines:
 * 1) Fixed global lines - initialized once during init()
 * 2) Global set of lines updated through Gobal Update Thread. Use this for displaying large line sets like object wireframes that need to change over time.
 * 3) local changes updated for each drawing thread. Only use for small line sets to bring dynamic element to line drawing. Lines have to be uploaded for each and every frame. Use rarely.
 */

 // basic line definitions
struct LineDef {
	glm::vec3 start, end;
	glm::vec4 color;
};

//class LineShader;
//struct LineShader::UniformBufferObject;
//class LineThreadResources;


// each execution needs one instance of ApplicationData
struct LineShaderApplicationData {
public:
	std::vector<LineDef> lines;
};

//struct LineThreadResources : ShaderThreadResources {
	//VkPipelineLayout pipelineLayout = nullptr;
	//VkPipeline graphicsPipeline = nullptr;
	//VkPipeline graphicsPipelineAdd = nullptr;
	//VkCommandBuffer commandBuffer = nullptr;
	//VkCommandBuffer commandBufferAdd = nullptr;
	//VkCommandBuffer commandBufferUpdate = nullptr;
	// vertex buffer for added lines
	//VkBuffer vertexBufferAdd = nullptr;
	// vertex buffer device memory
	//VkDeviceMemory vertexBufferAddMemory = nullptr;
	// MVP buffer
	//VkBuffer uniformBuffer = nullptr;
	//VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	//VkDeviceMemory uniformBufferMemory = nullptr;
	//VkDeviceMemory uniformBufferMemory2 = nullptr;
	//VkDescriptorSet descriptorSet = nullptr;
	//VkDescriptorSet descriptorSet2 = nullptr;
//};

class LineSubShader;
// line shader draws lines, it creates 2 pipelines, one for fixed lines (uploaded at start)
// and one for dynamic lines that change every frame
class LineShader : public ShaderBase {
public:
	std::vector<LineSubShader> globalLineSubShaders;
	std::vector<LineSubShader> perFrameLineSubShaders;

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
	void addGlobalConst(std::vector<LineDef>& linesToAdd);
	// global update: initiate update of global line data 
	// might take several frames until effect is visible: old buffer will be used until new one is ready
	void updateGlobal(std::vector<LineDef>& linesToAdd);
	// initial upload of all added lines - only valid before first render
	void initialUpload();

	// check if we need to switch resources for next render run
	void handleUpdatedResources(ThreadResources& tr);

	void createCommandBufferLineAdd(ThreadResources& tr);

	// clear line buffer, has to be called at begin of each frame
	// NOT after adding last group of lines
	void clearLocalLines(ThreadResources& tr);

	// per frame update of UBO / MVP
	void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO

	// resource switch after upload of new data has finished:
	void resourceSwitch(GlobalResourceSet set) override;
	std::vector<LineDef> lines;

	// global resources used by all LineSubShaders:
	// vertex buffer for fixed lines (one buffer for all threads) 
	VkBuffer vertexBuffer = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
private:
	//LineThreadResources globalLineThreadResources;
	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
	//// update cbuffer and vertex buffer
	//void update();
	//void updateUBO(UniformBufferObject newCBV);
	//// draw all lines in single call to GPU
	//void draw();
	//void destroy();


	int drawAddLinesSize = 0;

	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;
	// Inherited via Effect
	// set in init()

	//// vertex buffer for fixed lines (one buffer for all threads) 
	//VkBuffer vertexBuffer = nullptr;
	//// vertex buffer device memory
	//VkDeviceMemory vertexBufferMemory = nullptr;
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
	// add lines for just one frame
	void addOneTime(std::vector<LineDef>& linesToAdd, ThreadResources& tr);

	// prepare command buffer for added lines
	void prepareAddLines(ThreadResources& tr);

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

// manage all resources associated with ONE line drawing resource
// we have: Global (fixed after init), Local (only valid for one draw), 1-2 Update (updated sometimes according to app needs)
class LineResourceManager {

};

/*
 * LineSubShader includes everything for one shader invocation.
 * There will be 3 sub shaders: For fixed global lines, for global updated lines and for each frame a local one
 */
class LineSubShader {

public:
	// name is used in shader debugging
	void init(LineShader* parent, std::string debugName);
	void setVertShaderModule(VkShaderModule sm) {
		vertShaderModule = sm;
	}
	void setFragShaderModule(VkShaderModule sm) {
		fragShaderModule = sm;
	}
	void initSingle(ThreadResources& tr, ShaderState& shaderState);
	//void setResources(LineThreadResources* resources) {
	//	lineThreadResources = resources;
	//}
	void setVulkanResources(VulkanResources* vr) {
		vulkanResources = vr;
	}
	// add resources needed for per frame added lines
	void addPerFrameResources(ThreadResources& tr);

	void initialUpload();

	// all sections need: buffer allocation and recording draw commands.
	// stage they are called at will be very different
	void allocateCommandBuffer(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName);
	void addRenderPassAndDrawCommands(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer);

	void createGlobalCommandBufferAndRenderPass(ThreadResources& tr);
	void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
	// per frame update of UBO / MVP
	void uploadToGPU(ThreadResources& tr, LineShader::UniformBufferObject& ubo);
	void uploadToGPUAddedLines(ThreadResources& tr, LineShader::UniformBufferObject& ubo);

	void destroy();

	// gradually move ThreadResources here:
	VkCommandBuffer commandBuffer = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkFramebuffer framebuffer = nullptr;
	//VkFramebuffer framebuffer2 = nullptr;
	//VkFramebuffer framebufferAdd = nullptr;
	//VkFramebuffer framebufferAdd2 = nullptr;
	VkRenderPass renderPass = nullptr;
	//VkRenderPass renderPassAdd = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	// additional per frame resources
	VkBuffer vertexBufferAdd = nullptr;
	VkDeviceMemory vertexBufferAddMemory = nullptr;
	VkCommandBuffer commandBufferAdd = nullptr;
	std::vector<LineShader::Vertex> verticesAddLines;

private:
	LineShader* lineShader = nullptr;
	//LineThreadResources* lineThreadResources = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice* device = nullptr;
};

