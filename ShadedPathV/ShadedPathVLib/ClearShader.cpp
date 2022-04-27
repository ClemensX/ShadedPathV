#include "pch.h"

void ClearShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);

    // load shader binary code
    vector<byte> file_buffer_vert;
    vector<byte> file_buffer_frag;
    engine.files.readFile("triangle_vert.spv", file_buffer_vert, FileCategory::FX);
    engine.files.readFile("triangle_frag.spv", file_buffer_frag, FileCategory::FX);
    Log("read vertex shader: " << file_buffer_vert.size() << endl);
    Log("read fragment shader: " << file_buffer_frag.size() << endl);
    // create shader modules
    vertShaderModuleTriangle = createShaderModule(file_buffer_vert);
    fragShaderModuleTriangle = createShaderModule(file_buffer_frag);

    ThemedTimer::getInstance()->start(TIMER_PART_BUFFER_COPY);
    // create vertex buffer
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    engine.global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, vertices.data(), vertexBufferTriangle, vertexBufferMemoryTriangle);
    ThemedTimer::getInstance()->stop(TIMER_PART_BUFFER_COPY);

    // create index buffer
    bufferSize = sizeof(indices[0]) * indices.size();
    engine.global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bufferSize, indices.data(), indexBufferTriangle, indexBufferMemoryTriangle);

    // descriptor set layout
    createDescriptorSetLayout();

    // load texture
    engine.textureStore.loadTexture("debug.ktx", "debugTexture");
    texture = engine.textureStore.getTexture("debugTexture");

	// descriptor pool
	vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;
	createDescriptorPool(poolSizes);
}

void ClearShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	// uniform buffer
	createUniformBuffer(tr, tr.uniformBufferTriangle, sizeof(UniformBufferObject), tr.uniformBufferMemoryTriangle);

	createDescriptorSets(tr);
	createRenderPassAndFramebuffer(tr, shaderState, tr.renderPassSimpleShader, tr.framebuffer);

	// create shader stage
	auto vertShaderStageInfo = createVertexShaderCreateInfo(vertShaderModuleTriangle);
	auto fragShaderStageInfo = createFragmentShaderCreateInfo(fragShaderModuleTriangle);

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = getBindingDescription();
	auto attribute_desc = getAttributeDescriptions();
	auto vertexInputInfo = createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());
	//for (int i = 0; i < vertexInputInfo.vertexAttributeDescriptionCount; i++) {
	//	LogF(" offset " << vertexInputInfo.pVertexAttributeDescriptions[i].offset << endl);
	//	LogF(" format " << vertexInputInfo.pVertexAttributeDescriptions[i].format << endl);
	//}
	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = createStandardRasterizer();

	// multisampling
	auto multisampling = createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	createPipelineLayout(&tr.pipelineLayoutTriangle);

	// depth stencil
	auto depthStencil = createStandardDepthStencil();

	// create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = tr.pipelineLayoutTriangle;
	pipelineInfo.renderPass = tr.renderPassSimpleShader;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &tr.graphicsPipelineTriangle) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void ClearShader::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
}

void ClearShader::createDescriptorSets(ThreadResources& res)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &res.descriptorSetTriangle) != VK_SUCCESS) {
        Error("failed to allocate descriptor sets!");
    }

    // populate descriptor set:
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = res.uniformBufferTriangle;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->imageView;
    imageInfo.sampler = global->textureSampler;

    array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = res.descriptorSetTriangle;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = res.descriptorSetTriangle;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void ClearShader::createCommandBuffer(ThreadResources& tr)
{
	if (!enabled) return;
	auto& device = this->engine->global.device;
	auto& global = this->engine->global;
	auto& shaders = this->engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferClear) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(tr.commandBufferClear, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderingAttachmentInfoKHR color_attachment_info{};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	color_attachment_info.imageView = tr.colorAttachment.view;
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info.clearValue = clearValues[0];

	VkRenderingAttachmentInfoKHR depth_attachment_info{};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depth_attachment_info.imageView = tr.depthImageView;
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment_info.clearValue = clearValues[1];

	// Transition color attachment image to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	VkImageMemoryBarrier dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcAccessMask = 0;
	dstBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	dstBarrier.image = tr.colorAttachment.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferClear, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

	// Transition depth attachment image to LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcAccessMask = 0;
	dstBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	dstBarrier.image = tr.depthImage;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferClear, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

	//auto render_area = VkRect2D{ VkOffset2D{}, VkExtent2D{width, height} };
	//auto render_info = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
	VkRenderingInfo render_info{};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.colorAttachmentCount = 1;
	render_info.flags = 0;
	render_info.layerCount = 1;
	render_info.pColorAttachments = &color_attachment_info;
	render_info.pDepthAttachment = &depth_attachment_info;
	render_info.pStencilAttachment = &depth_attachment_info;
	render_info.renderArea.offset = { 0, 0 };
	render_info.renderArea.extent = this->engine->getBackBufferExtent();

	vkCmdBeginRendering(tr.commandBufferClear, &render_info);

	vkCmdEndRendering(tr.commandBufferClear);


	if (vkEndCommandBuffer(tr.commandBufferClear) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}

}

void ClearShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.commandBufferTriangle);
};


void ClearShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, VkBuffer indexBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr.graphicsPipelineTriangle);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	// bind descriptor sets:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr.pipelineLayoutTriangle, 0, 1, &tr.descriptorSetTriangle, 0, nullptr);

	//vkCmdDraw(commandBuffer, static_cast<uint32_t>(simpleShader.vertices.size()), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

ClearShader::~ClearShader()
{
	if (!enabled) {
		return;
	}
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyBuffer(device, vertexBufferTriangle, nullptr);
	vkFreeMemory(device, vertexBufferMemoryTriangle, nullptr);
	vkDestroyBuffer(device, indexBufferTriangle, nullptr);
	vkFreeMemory(device, indexBufferMemoryTriangle, nullptr);
	vkDestroyShaderModule(device, fragShaderModuleTriangle, nullptr);
	vkDestroyShaderModule(device, vertShaderModuleTriangle, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}
