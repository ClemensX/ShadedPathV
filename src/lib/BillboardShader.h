#pragma once
// Billboards - draw simple billboards in world coordinates with direction and size
// stages: VertexShader --> GeometryShader --> FragmentShader
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/

// BillboardDef is used in applicastion code to define the billboards AND directly used as Vertex definition
struct BillboardDef {
	glm::vec4 pos;
	glm::vec4 dir;  // direction for rotating a standard billboard to. Will be auto-reformulated as a quaternion before shader is called
	                // (0.5 0.5 0.5) would turn the billboard towards point (1, 1, 1)
	float w;
	float h;
	int type;  // 0 == face camera, 1 == use provided direction vector
	unsigned int textureIndex = 0; // index to global texture array
	//TextureID tex;
};

// make sure to match the push_constant layout in the shader
struct BillboardPushConstants {
    float worldSizeOneEdge = 0.0f; // world size in meters, used for both dimensions (x and z)
	int heightmapTextureIndex;
};

const VkPushConstantRange billboardPushConstantRange = {
    VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
    0, // offset
    sizeof(BillboardPushConstants) // size
};

// per frame resources for this effect
struct BillboardFrameData {
public:
	std::vector<BillboardDef> addBillboards;
};

// forward
class BillboardSubShader;

// Billboard shader draws billboards, it creates 2 pipelines, one for fixed (uploaded at start)
// and one for dynamic billoards that change every frame
class BillboardShader : public ShaderBase {
public:
	// max # billboards: we only allow adding billboards during init phase so we know the buffer size when reserving GPU memory

	std::vector<BillboardSubShader> globalSubShaders;

	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::GlobalTextureSet },
		{ VulkanResourceType::VertexBufferStatic }
	};

	// define billboard size and pos, --> to GPU as single UBO with all Billboards mem mapped
	typedef BillboardDef Vertex;

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
	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
		// layout(location = 0) in vec3 inPosition;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		// layout(location = 1) in vec3 direction;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, dir);
		// layout(location = 2) in float width;
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, w);
		// layout(location = 3) in float height;
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, h);
		// layout(location = 4) in int inType; // billboard type: 0 is towards camera, 1 is absolute inDirection
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[4].offset = offsetof(Vertex, type);
		// layout(location = 5) in int textureIndex; // global texture array index
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[5].offset = offsetof(Vertex, textureIndex);
		return attributeDescriptions;
	}
	virtual ~BillboardShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
	virtual void createCommandBuffer(FrameResources& tr) override;
	virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;

	// add billboards - they will never  be removed
	void add(std::vector<BillboardDef>& billboardsToAdd);
	// initial upload of all added billboards - only valid before first render
	void initialUpload();

	// Getter for heightmapTextureIndex
	int getHeightmapTextureIndex() const {
		return heightmapTextureIndex;
	}

	// Setter for heightmapTextureIndex
	void setHeightmapTextureIndex(int index) {
		heightmapTextureIndex = index;
	}

	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
	// calc verstices around origin needed to display billboard (used e.g. for debugging)
	// usually this is done in geom shader, not here
	static void calcVertsOrigin(std::vector<glm::vec3>& verts);
	// vertex buffer for fixed billboards (one buffer for all threads) 
	VkBuffer vertexBuffer = nullptr;
	int heightmapTextureIndex = -1;
	std::vector<BillboardDef> billboards;
private:
	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;

	// vertex buffer device memory
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	VkShaderModule geomShaderModule = nullptr;

	// util methods
public:
};

/*
 * BillboardSubShader includes everything for one shader invocation.
 * Currently only 1 sub shader
 */
class BillboardSubShader {
public:
	void init(BillboardShader* parent, std::string debugName);
	void setVertShaderModule(VkShaderModule sm) {
		vertShaderModule = sm;
	}
	void setGeomShaderModule(VkShaderModule sm) {
		geomShaderModule = sm;
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
	void uploadToGPU(FrameResources& tr, BillboardShader::UniformBufferObject& ubo, BillboardShader::UniformBufferObject& ubo2);

	void destroy();
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	// vertex buffer for billboards 
	// currently: only global buffer for all threads, no need to have per thread vertices
	//VkBuffer billboardVertexBuffer = nullptr;
	// vertex buffer device memory
	//VkDeviceMemory billboardVertexBufferMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
private:
	BillboardShader* billboardShader = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule geomShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	FrameResources* frameResources = nullptr;
};

struct BillboardThreadResources : ShaderThreadResources {
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	// vertex buffer for billboards 
	// currently: only global buffer for all threads, no need to have per thread vertices
	//VkBuffer billboardVertexBuffer = nullptr;
	// vertex buffer device memory
	//VkDeviceMemory billboardVertexBufferMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
};
