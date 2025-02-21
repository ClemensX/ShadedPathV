#pragma once

// forward
class CubeSubShader;

// Skybox cube shader
class CubeShader : public ShaderBase {
public:
	std::vector<CubeSubShader> globalSubShaders;

	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::SingleTexture },
		{ VulkanResourceType::VertexBufferStatic }
	};
	// Vertex is kind of fake as we do not need the actual vertex positions. They are const in the shader
	struct Vertex {
		glm::vec3 pos;
	};
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		float farFactor; // bloat factor for skybox cube
		bool outside; // make upside down for outside view
	};

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	// get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
		// layout(location = 0) in vec3 inPosition;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		return attributeDescriptions;
	}
	virtual ~CubeShader() override;
	// shader initialization, end result is a graphics pipeline for each ThreadResources instance

	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// thread resources initialization
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
	// create command buffers. One time auto called before rendering starts
	// we create one command buffer for every mesh loaded
	virtual void createCommandBuffer(FrameResources& tr) override;
	// add the pre-computed command buffer for the current object
	virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;

	// per frame update of UBOs / MVPs
	// outsideMode is for rendering a cube with cubemaps projected to its 6 sides
	void uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2, bool outsideMode = false);

	// set skybox cube texture id. The corresponding cube texture has to be loaded before calling this.
	void setSkybox(std::string texID);

	// set far plane, user to calculate size of skybox
	void setFarPlane(float farPlane) {
		bloatFactor = Util::getMaxCubeViewDistanceFromFarPlane(farPlane) - 1.0f;
	}
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	TextureInfo* skybox = nullptr;
	// we need a buffer to keep validation happy - content is irrelevant
	Vertex verts_fake_buffer[36];
	float bloatFactor = 1.0f;
private:
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, bool isRightEye = false);
	void createSkyboxTextureDescriptors();
	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;


};

/*
 * CubeSubShader includes everything for one shader invocation.
 * Currently only 1 sub shader
 */
class CubeSubShader {
public:
	// name is used in shader debugging
	void init(CubeShader* parent, std::string debugName);
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
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, bool isRightEye = false);
	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, CubeShader::UniformBufferObject& ubo, CubeShader::UniformBufferObject& ubo2, bool outsideMode = false);

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
	VkPipelineLayout pipelineLayout = nullptr;

private:
	CubeShader* cubeShader = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	FrameResources* frameResources = nullptr;
};
