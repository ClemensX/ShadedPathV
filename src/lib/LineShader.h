#pragma once

/*
 * Line Shader - draw various lines:
 * 1) Fixed global lines - initialized once during init()
 * 2) Global set of lines updated through Gobal Update Thread. Use this for displaying large line sets like object wireframes that need to change over time.
 * 3) local changes updated for each drawing thread. Only use for small line sets to bring dynamic element to line drawing. Lines have to be uploaded for each and every frame. Use rarely.
 */

 // basic line definitions, see globalDef.h
//struct LineDef {
//	glm::vec3 start, end;
//	glm::vec4 color;
//};

// each execution needs one instance of ApplicationData
struct LineShaderApplicationData {
public:
	std::vector<LineDef> lines;
};

struct LineShaderUpdateElement {
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	size_t drawCount = 0;
	bool active = false; // store usage state, set by global update thread, reset by render thread
};

// forward
class LineSubShader;

class LineShader : public ShaderBase {
public:
	std::vector<LineSubShader> globalLineSubShaders;
	std::vector<LineSubShader> perFrameLineSubShaders;
	std::vector<LineSubShader> globalUpdateLineSubShaders;

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

	// max # lines for dynamic adding for single frame
	// we limit this to allow for pre-allocated vertex buffer in thread resources
	//static const size_t MAX_DYNAMIC_LINES = 100000;
	static const size_t MAX_DYNAMIC_LINES = 9000000;

	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
	virtual void createCommandBuffer(FrameResources& tr) override;
	virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;


	// add lines - they will never  be removed
	void addFixedGlobalLines(std::vector<LineDef>& linesToAdd);
	// upload fixed global lines - only valid before first render
	void uploadFixedGlobalLines();

	// clear line buffer, has to be called at begin of each frame
	// NOT after adding last group of lines
	void clearLocalLines(FrameResources& tr);

	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO

	std::vector<LineDef> lines;

	// global resources used by all LineSubShaders:
	// vertex buffer for fixed lines (one buffer for all threads) 
	VkBuffer vertexBufferFixedGlobal = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemoryFixedGlobal = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	// Resources for permamnent lines:

	// called from app.prepareFrame(), we are single threaded there
	void applyGlobalUpdate(FrameResources& tr);
	std::vector<LineShader::Vertex> verticesPermanent;

private:
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
    LineShaderUpdateElement updateElement = {}; // copied to activeUpdateElement in sub shader

	int drawAddLinesSize = 0;

	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;
	// Inherited via Effect
	// set in init()

	// vertex buffer for updates: (one buffer for all threads) 
	VkBuffer vertexBufferUpdates = nullptr;
	// vertex buffer device memory for Updates
	VkDeviceMemory vertexBufferMemoryUpdates = nullptr;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
    int oldUpdateInUse = 0; // count down from 2 to 0, then we can free old update resources because no old update element is still in use

	// util methods
public:
	// add lines for just one frame
	void addOneTime(std::vector<LineDef>& linesToAdd, FrameResources& tr);

	// prepare command buffer for added lines
	void prepareAddLines(FrameResources& tr);

	// add lines permanently via update thread
	void addPermament(std::vector<LineDef>& linesToAdd);

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
	void initSingle(FrameResources& tr, ShaderState& shaderState);
	void setVulkanResources(VulkanResources* vr) {
		vulkanResources = vr;
	}

	// All sections need: buffer allocation and recording draw commands.
	// Stage they are called at will be very different
	void allocateCommandBuffer(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName);
	void addRenderPassAndDrawCommands(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer);

	void createGlobalCommandBufferAndRenderPass(FrameResources& tr);
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);
	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, LineShader::UniformBufferObject& ubo, LineShader::UniformBufferObject& ubo2);

	void destroy();

	// gradually move ThreadResources here:
	VkCommandBuffer commandBuffer = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	// additional per frame resources
	std::vector<LineShader::Vertex> vertices;
	size_t drawCount = 0; // set number of draw calls for this sub shader, also used as indicator if this is active
	// for cases where vertex buffer is stored in sub shader:
	VkBuffer vertexBufferLocal = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemoryLocal = nullptr;
	bool active = false;
	LineShaderUpdateElement activeUpdateElement = {}; // vertex buffer and memory in use for global update

private:
	LineShader* lineShader = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice* device = nullptr;
	FrameResources* frameResources = nullptr;
};

