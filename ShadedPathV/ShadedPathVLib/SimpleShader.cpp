#include "pch.h"

using namespace std;

void SimpleShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

    // create shader modules
    vertShaderModuleTriangle = resources.createShaderModule("triangle_vert.spv");
    fragShaderModuleTriangle = resources.createShaderModule("triangle_frag.spv");

    // create vertex buffer
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	resources.createVertexBufferStatic(VertexBufferId, bufferSize, vertices.data(), vertexBufferTriangle, vertexBufferMemoryTriangle);

    // create index buffer
    bufferSize = sizeof(indices[0]) * indices.size();
    resources.createIndexBufferStatic(IndexBufferId, bufferSize, indices.data(), indexBufferTriangle, indexBufferMemoryTriangle);

    // descriptor set layout
    resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);

    // load texture
	//engine.textureStore.loadTexture("debug.ktx", "debugTexture");
	engine.textureStore.loadTexture("dump.ktx", "debugTexture");
	//engine.textureStore.loadTexture("arches_pinetree_low.ktx2", "debugTexture");
	//texture = engine.textureStore.getTexture("debugTexture");
	texture = engine.textureStore.getTexture(engine.textureStore.BRDFLUT_TEXTURE_ID);
}

void SimpleShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	VulkanHandoverResources handover;
	auto& str = tr.simpleResources; //shortcut to shader thread resources
	// MVP uniform buffer
	createUniformBuffer(tr, str.uniformBuffer, sizeof(UniformBufferObject), str.uniformBufferMemory);
	handover.mvpBuffer = str.uniformBuffer;
	handover.mvpSize = sizeof(UniformBufferObject);
	handover.imageView = texture->imageView;
	handover.descriptorSet = &str.descriptorSet;
	resources.createThreadResources(handover);
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModuleTriangle);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModuleTriangle);

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
	createPipelineLayout(&str.pipelineLayout);

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
	pipelineInfo.layout = str.pipelineLayout;
	pipelineInfo.renderPass = str.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &str.graphicsPipeline) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void SimpleShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void SimpleShader::createDescriptorSetLayout()
{
    Error("remove this method from base class!");
}

void SimpleShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void SimpleShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo) {
	if (!enabled) Error("Shader disabled. Calling methods on it is not allowed.");
    // copy ubo to GPU:
    void* data;
    vkMapMemory(device, tr.simpleResources.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, tr.simpleResources.uniformBufferMemory);
}

void SimpleShader::createCommandBuffer(ThreadResources& tr)
{
	if (!enabled) return;
	auto& str = tr.simpleResources; //shortcut to shader thread resources
	auto& device = this->engine->global.device;
	auto& global = this->engine->global;
	auto& shaders = this->engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &str.commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(str.commandBuffer, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = str.renderPass;
	renderPassInfo.framebuffer = str.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->engine->getBackBufferExtent();

	//renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	//renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(str.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	recordDrawCommand(str.commandBuffer, tr, vertexBufferTriangle, indexBufferTriangle);
	vkCmdEndRenderPass(str.commandBuffer);
	if (vkEndCommandBuffer(str.commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void SimpleShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.simpleResources.commandBuffer);
};


void SimpleShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, VkBuffer indexBuffer)
{
	auto& str = tr.simpleResources; //shortcut to shader thread resources
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	// bind descriptor sets:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet, 0, nullptr);

	//vkCmdDraw(commandBuffer, static_cast<uint32_t>(simpleShader.vertices.size()), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

SimpleShader::~SimpleShader()
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

void SimpleShader::destroyThreadResources(ThreadResources& tr)
{
	auto& trl = tr.simpleResources;
	vkDestroyFramebuffer(device, trl.framebuffer, nullptr);
	vkDestroyRenderPass(device, trl.renderPass, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, trl.pipelineLayout, nullptr);
	vkDestroyBuffer(device, trl.uniformBuffer, nullptr);
	vkFreeMemory(device, trl.uniformBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, trl.framebuffer2, nullptr);
	}
}
