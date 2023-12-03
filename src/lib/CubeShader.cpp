#include "mainheader.h"

using namespace std;

void CubeShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("cube_vert.spv");
	fragShaderModule = resources.createShaderModule("cube_frag.spv");

	// we need a buffer to keep validation happy - content is irrelevant
	static Vertex verts[36];
	VkDeviceSize bufferSize = sizeof(Vertex) * 36;
	resources.createVertexBufferStatic(bufferSize, &(verts[0]), vertexBuffer, vertexBufferMemory);

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);
}

void CubeShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	auto& str = tr.cubeResources; // shortcut to cube resources

	// uniform buffer
	createUniformBuffer(tr, str.uniformBuffer, sizeof(UniformBufferObject), str.uniformBufferMemory);
	engine->util.debugNameObjectBuffer(str.uniformBuffer, "Cube UBO 1");
	engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory, "Cube Memory 1");
	if (engine->isStereo()) {
		createUniformBuffer(tr, str.uniformBuffer2, sizeof(UniformBufferObject), str.uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(str.uniformBuffer2, "Cube UBO 2");
		engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory2, "Cube Memory 2");
	}
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = getBindingDescription();
	auto attribute_desc = getAttributeDescriptions();
	auto vertexInputInfo = createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

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
	resources.createPipelineLayout(&str.pipelineLayout);
	//const std::vector<VkDescriptorSetLayout> setLayouts = {
	//		descriptorSetLayout
	//};
	//VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	//pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//pipelineLayoutInfo.setLayoutCount = 1;
	//pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	//pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	//pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	//if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &str.pipelineLayout) != VK_SUCCESS) {
	//	Error("failed to create pipeline layout!");
	//}

	//createPipelineLayout(&str.pipelineLayout);

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

void CubeShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void CubeShader::createDescriptorSetLayout()
{
	Error("remove this method from base class!");
}

void CubeShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void CubeShader::createCommandBuffer(ThreadResources& tr)
{
	auto& str = tr.cubeResources; // shortcut to cube resources
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	VulkanHandoverResources handover;
	handover.mvpBuffer = str.uniformBuffer;
	handover.mvpBuffer2 = str.uniformBuffer2;
	handover.mvpSize = sizeof(UniformBufferObject);
	handover.imageView = skybox->imageView;
	handover.descriptorSet = &str.descriptorSet;
	handover.descriptorSet2 = &str.descriptorSet2;
	resources.createThreadResources(handover);
	engine->util.debugNameObjectDescriptorSet(str.descriptorSet, "Cube Descriptor Set 1");
	if (engine->isStereo())	engine->util.debugNameObjectDescriptorSet(str.descriptorSet2, "Cube Descriptor Set 2");

	resources.updateDescriptorSets(tr);
	//createDescriptorSets(tr);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &str.commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(str.commandBuffer, "Cube COMMAND BUFFER");
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
	renderPassInfo.renderArea.extent = engine->getBackBufferExtent();

	renderPassInfo.clearValueCount = 0;

	vkCmdBeginRenderPass(str.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	recordDrawCommand(str.commandBuffer, tr);
	vkCmdEndRenderPass(str.commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = str.framebuffer2;
		vkCmdBeginRenderPass(str.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		recordDrawCommand(str.commandBuffer, tr, true);
		vkCmdEndRenderPass(str.commandBuffer);
	}
	if (vkEndCommandBuffer(str.commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void CubeShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.cubeResources.commandBuffer);
};

void CubeShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, bool isRightEye)
{
	auto& str = tr.cubeResources; // shortcut to cube resources
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// bind descriptor set
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet, 0, nullptr);
	}
	else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet2, 0, nullptr);
	}
	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void CubeShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2, bool outsideMode) {
	if (!enabled) return;
	auto& str = tr.cubeResources; // shortcut to cube resources
	ubo2.farFactor = ubo.farFactor = bloatFactor;
	ubo2.outside = ubo.outside = outsideMode;
	// copy ubo to GPU:
	void* data;
	vkMapMemory(device, str.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, str.uniformBufferMemory);
	if (engine->isStereo()) {
		vkMapMemory(device, str.uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, str.uniformBufferMemory2);
	}
}

void CubeShader::setSkybox(string texID)
{
	if (!enabled) return;
	skybox = engine->textureStore.getTexture(texID);
	if (skybox->vulkanTexture.viewType != VK_IMAGE_VIEW_TYPE_CUBE) {
		Error("Can only use textures with VK_IMAGE_VIEW_TYPE_CUBE for skybox / cube map");
	}
}

void CubeShader::createSkyboxTextureDescriptors()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	//allocInfo.pSetLayouts = &descriptorSetLayoutTexture;
	//VkResult res = vkAllocateDescriptorSets(device, &allocInfo, &mesh->descriptorSet);
	//if (res != VK_SUCCESS) {
	//	Error("failed to allocate descriptor sets!");
	//}
	//engine->util.debugNameObjectDescriptorSet(mesh->descriptorSet, "Mesh Texture Descriptor Set");
	//// Descriptor
	//array<VkWriteDescriptorSet, 1> descriptorWrites{};
	//// populate descriptor set:
	//VkDescriptorImageInfo imageInfo0{};
	//imageInfo0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//imageInfo0.imageView = mesh->textureInfos[0]->imageView;
	//imageInfo0.sampler = global->textureSampler;

	//descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//descriptorWrites[0].dstSet = mesh->descriptorSet;
	//descriptorWrites[0].dstBinding = 0;
	//descriptorWrites[0].dstArrayElement = 0;
	//descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//descriptorWrites[0].descriptorCount = 1;
	//descriptorWrites[0].pImageInfo = &imageInfo0;
	//vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

CubeShader::~CubeShader()
{
	Log("CubeShader destructor\n");
	if (!enabled) {
		return;
	}
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void CubeShader::destroyThreadResources(ThreadResources& tr)
{
	auto& str = tr.cubeResources; // shortcut to cube resources
	vkDestroyFramebuffer(device, str.framebuffer, nullptr);
	vkDestroyRenderPass(device, str.renderPass, nullptr);
	vkDestroyPipeline(device, str.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, str.pipelineLayout, nullptr);
	vkDestroyBuffer(device, str.uniformBuffer, nullptr);
	vkFreeMemory(device, str.uniformBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, str.framebuffer2, nullptr);
		vkDestroyBuffer(device, str.uniformBuffer2, nullptr);
		vkFreeMemory(device, str.uniformBufferMemory2, nullptr);
	}
}
