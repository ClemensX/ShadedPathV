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

	// create graphics pipeline with all support structures and other thread resources
	virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) = 0;

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

	// util methods to simplify shader creation
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

	// may not be suitable for all shaders - single fields may simply be overwritten
	VkPipelineRasterizationStateCreateInfo createStandardRasterizer() {
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		return rasterizer;
	}

	// may not be suitable for all shaders - single fields may simply be overwritten
	// provide ColorBlendAttachment to ease its assignment in returned structure. It does not have to be initialized.
	VkPipelineColorBlendStateCreateInfo createStandardColorBlending(VkPipelineColorBlendAttachmentState &colorBlendAttachment)  {
		VkPipelineColorBlendAttachmentState empty{};
		colorBlendAttachment = empty;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional
		return colorBlending;
	}

	// may not be suitable for all shaders - single fields may simply be overwritten
	VkPipelineMultisampleStateCreateInfo createStandardMultisampling() {
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional
		return multisampling;
	}

	// create pipeline layout and store in parameter
	void createPipelineLayout(VkPipelineLayout *pipelineLayout) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS) {
			Error("failed to create pipeline layout!");
		}
	}

	VkPipelineDepthStencilStateCreateInfo createStandardDepthStencil() {
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional
		return depthStencil;
	}
};
