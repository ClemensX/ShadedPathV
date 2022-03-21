#pragma once

// Info needed to connect shaders.
// Like DepthBuffer, sizes, image formats and states
struct ShaderState
{
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewportState{};

};

class ShaderBase
{
public:
	ShaderBase() {

	};

	// virtual methods each subclass has to provide

	virtual ~ShaderBase() = 0;

	// initializations:
	// Color and depth render pass (single)
	//   VkAttachmentDescription
	//   VkAttachmentReference
	//   VkAttachmentDescription
	//   VkAttachmentReference
	//   VkSubpassDependency
	//   VkSubpassDescription
	//   VkAttachmentDescription
	//   VkRenderPassCreateInfo
	//   --> vkCreateRenderPass
	// 
	// Uniform buffers (multi)
	//   VkBufferCreateInfo
	//   VkMemoryRequirements
	//   VkMemoryAllocateInfo
	//   --> vkAllocateMemory
	//   --> vkBindBufferMemory
	// 
	// color and depth framebuffer (multi)
	//   VkImageView
	//   VkFramebufferCreateInfo
	//   --> vkCreateFramebuffer
	// 
	// descriptor pool (single)
	//   VkDescriptorPoolSize
	//   VkDescriptorPoolCreateInfo
	//   --> vkCreateDescriptorPool
	// 
	// descriptor set (single and multi)
	//   VkDescriptorSetLayoutBinding (single)
	//   VkDescriptorSetLayoutCreateInfo (single)
	//   --> vkCreateDescriptorSetLayout (single)
	//   VkDescriptorSetLayout (single)
	//   VkDescriptorSetAllocateInfo (single)
	//   --> vkAllocateDescriptorSets (single)
	//     VkDescriptorSet (multi)
	//     VkDescriptorBufferInfo (multi)
	//     VkDescriptorImageInfo (multi)
	//     VkWriteDescriptorSet
	//     --> vkUpdateDescriptorSets (multi)
	// 
	// pipeline layout (single)
	//   VkPipelineLayoutCreateInfo
	//   --> vkCreatePipelineLayout
	// 
	// graphics pipeline (single)
	//   VkShaderStageFlagBits
	//   VkPipelineVertexInputStateCreateInfo
	// 	 VkPipelineInputAssemblyStateCreateInfo
	// 	 VkViewport
	// 	 VkRect2D
	// 	 VkPipelineViewportStateCreateInfo
	// 	 VkPipelineRasterizationStateCreateInfo
	// 	 VkPipelineMultisampleStateCreateInfo
	// 	 VkPipelineColorBlendAttachmentState
	// 	 VkPipelineColorBlendStateCreateInfo
	// 	 VkPipelineDepthStencilStateCreateInfo
	// 	 VkDynamicState
	// 	 VkPipelineDynamicStateCreateInfo
	// 	 VkPipelineTessellationStateCreateInfo
	// 	 VkGraphicsPipelineCreateInfo
	// 	 --> vkCreateGraphicsPipelines
	// 	 --> vkDestroyShaderModule
	//
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderSate) = 0;


	// create descriptor set layout and assign to BaseShader variable
	// (one per shader)
	virtual void createDescriptorSetLayout() = 0;

	// each shader must initialize its descriptor sets for all render threads
	virtual void createDescriptorSets(ThreadResources& res) = 0;

	// Base class methodas that can be used in the subclasses

	// common initializations, usually called as first step in subclass init()
	void init(ShadedPathEngine& engine);

	// each shader has its own descriptor pool
	// poolSizes will be auto-multiplied by number of render threads
	// to have enough descriptors for all threads
	void createDescriptorPool(vector<VkDescriptorPoolSize>& poolSizes);

	// Thread dependent intializations:

	// create UniformBuffer. Size has to be provided by subclasses (no clean way to do it via inheritance or CRTP)
	void createUniformBuffer(ThreadResources& res, VkBuffer& uniformBuffer, size_t size, VkDeviceMemory& uniformBufferMemory);

protected:
	// signal if this shader is in use, set during init()
	bool enabled = false;
	// initialized in init()
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	GlobalRendering* global = nullptr;

	VkShaderModule createShaderModule(const vector<byte>& code);
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorPool descriptorPool = nullptr;

	// shader create info util methods
	VkPipelineShaderStageCreateInfo createVertexShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaderModule;
		vertShaderStageInfo.pName = "main";
		return vertShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo createFragmentShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaderModule;
		fragShaderStageInfo.pName = "main";
		return fragShaderStageInfo;
	}

	VkPipelineVertexInputStateCreateInfo createVertexInputCreateInfo(VkVertexInputBindingDescription* vertexInputBinding, VkVertexInputAttributeDescription* vertexInputAttributes, size_t attributes_size) {
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = vertexInputBinding;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t> (attributes_size);
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes;
		return vertexInputInfo;
	}

};

