#include "mainheader.h"

using namespace std;

void CubeShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("cube.vert.spv");
	fragShaderModule = resources.createShaderModule("cube.frag.spv");

	VkDeviceSize bufferSize = sizeof(Vertex) * 36;
	resources.createVertexBufferStatic(bufferSize, &(verts_fake_buffer[0]), vertexBuffer, vertexBufferMemory);

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, this, 1);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		// global fixed lines (one common vertex buffer)
		CubeSubShader sub;
		sub.init(this, "CubeSubShader");
		sub.setVertShaderModule(vertShaderModule);
		sub.setFragShaderModule(fragShaderModule);
		sub.setVulkanResources(&resources);
		globalSubShaders.push_back(sub);
	}
}

void CubeShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	CubeSubShader& ug = globalSubShaders[tr.frameIndex];
	ug.initSingle(tr, shaderState);
}

void CubeSubShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	// uniform buffer
	cubeShader->createUniformBuffer(uniformBuffer, sizeof(CubeShader::UniformBufferObject), uniformBufferMemory);
	engine->util.debugNameObjectBuffer(uniformBuffer, "Cube UBO 1");
	engine->util.debugNameObjectDeviceMmeory(uniformBufferMemory, "Cube Memory 1");
	if (engine->isStereo()) {
		cubeShader->createUniformBuffer(uniformBuffer2, sizeof(CubeShader::UniformBufferObject), uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(uniformBuffer2, "Cube UBO 2");
		engine->util.debugNameObjectDeviceMmeory(uniformBufferMemory2, "Cube Memory 2");
	}
	cubeShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = cubeShader->getBindingDescription();
	auto attribute_desc = cubeShader->getAttributeDescriptions();
	auto vertexInputInfo = cubeShader->createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = cubeShader->createStandardRasterizer();

	// multisampling
	auto multisampling = cubeShader->createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = cubeShader->createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	vulkanResources->createPipelineLayout(&pipelineLayout, cubeShader);
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
	auto depthStencil = cubeShader->createStandardDepthStencil();

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
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void CubeShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void CubeShader::createCommandBuffer(FrameResources& tr)
{
	CubeSubShader& sub = globalSubShaders[tr.frameIndex];
	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void CubeSubShader::createGlobalCommandBufferAndRenderPass(FrameResources& tr)
{
	auto& device = engine->globalRendering.device;
	auto& global = engine->globalRendering;
	auto& shaders = engine->shaders;
	VulkanHandoverResources handover;
	handover.mvpBuffer = uniformBuffer;
	handover.mvpBuffer2 = uniformBuffer2;
	handover.mvpSize = sizeof(CubeShader::UniformBufferObject);
	handover.imageView = cubeShader->skybox->imageView;
	handover.descriptorSet = &descriptorSet;
	handover.descriptorSet2 = &descriptorSet2;
	handover.shader = cubeShader;
	vulkanResources->createThreadResources(handover);
	engine->util.debugNameObjectDescriptorSet(descriptorSet, "Cube Descriptor Set 1");
	if (engine->isStereo())	engine->util.debugNameObjectDescriptorSet(descriptorSet2, "Cube Descriptor Set 2");

	vulkanResources->updateDescriptorSets(tr);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(commandBuffer, "Cube COMMAND BUFFER");
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = engine->getBackBufferExtent();

	renderPassInfo.clearValueCount = 0;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	recordDrawCommand(commandBuffer, tr);
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		recordDrawCommand(commandBuffer, tr, true);
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void CubeShader::addCurrentCommandBuffer(FrameResources& tr) {
};

void CubeShader::addCommandBuffers(FrameResources* fr, DrawResult* drawResult) {
	int index = drawResult->getNextFreeCommandBufferIndex();
	auto& sub = globalSubShaders[fr->frameIndex];
	drawResult->commandBuffers[index++] = sub.commandBuffer;
}

void CubeShader::uploadToGPU(FrameResources& fr, UniformBufferObject& ubo, UniformBufferObject& ubo2, bool outsideMode) {
	if (!enabled) return;
	auto& sub = globalSubShaders[fr.frameIndex];
	sub.uploadToGPU(fr, ubo, ubo2, outsideMode);
}

void CubeSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, bool isRightEye)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	VkBuffer vertexBuffers[] = { cubeShader->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// bind descriptor set
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	}
	else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet2, 0, nullptr);
	}
	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void CubeSubShader::uploadToGPU(FrameResources& tr, CubeShader::UniformBufferObject& ubo, CubeShader::UniformBufferObject& ubo2, bool outsideMode) {
	ubo2.farFactor = ubo.farFactor = cubeShader->bloatFactor;
	ubo2.outside = ubo.outside = outsideMode;
	// copy ubo to GPU:
	void* data;
	vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBufferMemory);
	if (engine->isStereo()) {
		vkMapMemory(device, uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, uniformBufferMemory2);
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
	for (CubeSubShader& sub : globalSubShaders) {
		sub.destroy();
	}
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void CubeShader::destroyThreadResources(FrameResources& tr)
{
}

void CubeSubShader::init(CubeShader* parent, std::string debugName) {
	cubeShader = parent;
	name = debugName;
	engine = cubeShader->engine;
	device = engine->globalRendering.device;
	Log("CubeSubShader init: " << debugName.c_str() << std::endl);
}

void CubeSubShader::destroy()
{
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, framebuffer2, nullptr);
		vkDestroyBuffer(device, uniformBuffer2, nullptr);
		vkFreeMemory(device, uniformBufferMemory2, nullptr);
	}
}
