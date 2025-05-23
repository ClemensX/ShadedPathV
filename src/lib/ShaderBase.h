#pragma once

// forward
class ShaderBase;
class Config;
struct ShaderState;

// Thread local resources
struct ShaderThreadResources {
};

// Info needed to connect shaders during intialization.
// Like DepthBuffer, sizes, image formats and states
class ShaderBase
{
public:
	ShaderBase() {
	};

	// virtual methods each subclass has to provide

	virtual ~ShaderBase() = 0;

	virtual std::string getName() const {
		return typeid(*this).name();
	}

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

	// initialize shader
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderSate) = 0;

	// create graphics pipeline with all support structures and other thread resources
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) = 0;

	// finish shader initialization
	virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderSate) {};

	// destroy thread local shader resources
	virtual void destroyThreadResources(FrameResources& tr) {};

	// create command buffers. One time auto called before rendering starts.
	// Also post init phase stuff goes here, like VulcanResources.updateDescriptorSets()
	virtual void createCommandBuffer(FrameResources& tr) = 0;

	// add current command buffers
    virtual void addCommandBuffers(FrameResources* tr, DrawResult* drawResult) = 0;

	// Base class methodas that can be used in the subclasses

	// common initializations, usually called as first step in subclass init()
	void init(ShadedPathEngine& engine);

	// each shader has its own descriptor pool
	// poolSizes will be auto-multiplied by number of render threads
	// to have enough descriptors for all threads
	void createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t overrideMaxSet = 0);
	// each shader has its own descriptor pool
	// poolSizes will be auto-multiplied by number of render threads
	// to have enough descriptors for all threads
	// additionally a set of non-thread related pool sizes can be specified
	void createDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes, std::vector<VkDescriptorPoolSize>& threadIndependentPoolSizes, uint32_t overrideMaxSet = 0);

	// Thread dependent intializations:

	// create UniformBuffer. Size has to be provided by subclasses (no clean way to do it via inheritance or CRTP)
	// buffer GPU memory is mapped to uniformBufferMemory for CPU access
	void createUniformBuffer(VkBuffer& uniformBuffer, size_t size, VkDeviceMemory& uniformBufferMemory);

	// create VertexBuffer. buffer GPU memory is mapped to uniformBufferMemory for CPU access
	void createVertexBuffer(VkBuffer& uniformBuffer, size_t size, VkDeviceMemory& uniformBufferMemory);

	// create render pass and framebuffer with respect to shader state
	void createRenderPassAndFramebuffer(FrameResources& tr, ShaderState shaderState, VkRenderPass& renderPass, VkFramebuffer& frameBuffer, VkFramebuffer& frameBuffer2);

	void setLastShader(bool last) {
		lastShader = last;
	}

	bool isLastShader() {
		return lastShader;
	}

	void setWireframe(bool wireframe = true) {
		this->wireframe = wireframe;
    }
	//protected:
	// signal if this shader is in use, set during init()
	bool enabled = false;
	bool lastShader = false;
	// initialized in init()
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	GlobalRendering* global = nullptr;
	VulkanResources resources;
	bool wireframe = false;

	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorPool descriptorPool = nullptr;
	// debug pool allocation count
    uint32_t poolAllocationSetCount = 0; // increase on each set allocation
    uint32_t poolAllocationMaxSet = 0; // maximum number of sets allocated with vkCreateDescriptorPool()
    uint32_t poolAllocationMaxSingleDescriptors = 0; // cumulated number of single descriptors over all descrptor sets (all descriptor types added together)
	std::vector<VkPushConstantRange> pushConstantRanges;

	// util methods to simplify shader creation
	VkPipelineVertexInputStateCreateInfo createVertexInputCreateInfo(VkVertexInputBindingDescription* vertexInputBinding, const VkVertexInputAttributeDescription* vertexInputAttributes, size_t attributes_size) {
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = vertexInputBinding;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t> (attributes_size);
		vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes;
		return vertexInputInfo;
	}

	// may not be suitable for all shaders - single fields may simply be overwritten
	VkPipelineRasterizationStateCreateInfo createStandardRasterizer();

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

	// create pipeline layout and store in parameter.
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
		depthStencil.minDepthBounds = 0.0f; // Optional, not used
		depthStencil.maxDepthBounds = 1.0f; // Optional, not used
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional
		return depthStencil;
	}
};

