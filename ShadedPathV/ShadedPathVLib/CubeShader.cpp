#include "pch.h"

void CubeShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	engine.files.readFile("cube_vert.spv", file_buffer_vert, FileCategory::FX);
	engine.files.readFile("cube_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	vertShaderModule = engine.shaders.createShaderModule(file_buffer_vert);
	fragShaderModule = engine.shaders.createShaderModule(file_buffer_frag);

	// descriptor
	createDescriptorSetLayout();

	// descriptor pool
	// 3 buffers: V,P matrices, line data and dynamic uniform buffer for model matrix
	// line data is global buffer
	vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.resize(1);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	createDescriptorPool(poolSizes, 0);
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
	//createDescriptorSets(tr);
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = createFragmentShaderCreateInfo(fragShaderModule);
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
	const std::vector<VkDescriptorSetLayout> setLayouts = {
			descriptorSetLayout
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &str.pipelineLayout) != VK_SUCCESS) {
		Error("failed to create pipeline layout!");
	}

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

void CubeShader::initialUpload()
{
	// we need a buffer to keep validation happy - content is irrelevant
	static Vertex verts[36];
	VkDeviceSize bufferSize = sizeof(Vertex) * 36;
	global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, &(verts[0]), vertexBuffer, vertexBufferMemory);

}

void CubeShader::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		Error("failed to create descriptor set layout!");
	}
	engine->util.debugNameObjectDescriptorSetLayout(descriptorSetLayout, "Cube Descriptor Set Layout");
}

void CubeShader::createDescriptorSets(ThreadResources& tr)
{
	auto& str = tr.cubeResources; // shortcut to cube resources
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	if (vkAllocateDescriptorSets(device, &allocInfo, &str.descriptorSet) != VK_SUCCESS) {
		Error("failed to allocate descriptor sets!");
	}
	engine->util.debugNameObjectDescriptorSet(str.descriptorSet, "Cube Descriptor Set");

	array<VkWriteDescriptorSet, 2> descriptorWrites{};
	// populate descriptor set:
	VkDescriptorBufferInfo bufferInfo0{};
	bufferInfo0.buffer = str.uniformBuffer;
	bufferInfo0.offset = 0;
	bufferInfo0.range = sizeof(UniformBufferObject);

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = str.descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo0;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = skybox->imageView;
	imageInfo.sampler = global->textureSampler;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = str.descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	if (engine->isStereo()) {
		if (vkAllocateDescriptorSets(device, &allocInfo, &str.descriptorSet2) != VK_SUCCESS) {
			Error("failed to allocate descriptor sets!");
		}
		engine->util.debugNameObjectDescriptorSet(str.descriptorSet, "Cube Descriptor Set 2");
		bufferInfo0.buffer = str.uniformBuffer2;
		descriptorWrites[0].dstSet = str.descriptorSet2;
		descriptorWrites[1].dstSet = str.descriptorSet2;
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

}

void CubeShader::createCommandBuffer(ThreadResources& tr)
{
	auto& str = tr.cubeResources; // shortcut to cube resources
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	createDescriptorSets(tr);
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

void CubeShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	auto& str = tr.cubeResources; // shortcut to cube resources
	ubo.farFactor = bloatFactor;
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
	skybox = engine->textureStore.getTexture(texID);
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
