#include "pch.h"

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	engine.files.readFile("line_vert.spv", file_buffer_vert, FileCategory::FX);
	engine.files.readFile("line_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	vertShaderModule = engine.shaders.createShaderModule(file_buffer_vert);
	fragShaderModule = engine.shaders.createShaderModule(file_buffer_frag);

	// descriptor
	createDescriptorSetLayout();

	// descriptor pool
	// 2 buffers: MVP matrix and line data
	// line data is global buffer
	vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.resize(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = 1;
	createDescriptorPool(poolSizes);
}

void LineShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	auto& trl = tr.lineResources;
	// uniform buffer
	createUniformBuffer(tr, trl.uniformBuffer, sizeof(UniformBufferObject), trl.uniformBufferMemory);
	if (engine->isStereo()) {
		createUniformBuffer(tr, trl.uniformBuffer2, sizeof(UniformBufferObject), trl.uniformBufferMemory2);
	}

	createDescriptorSets(tr);
	// TODO remove hack
	bool undoLast = false;
	if (isLastShader()) {
		undoLast = true;
		setLastShader(false);
	}
	createRenderPassAndFramebuffer(tr, shaderState, trl.renderPass, trl.framebuffer, trl.framebuffer2);
	if (undoLast) {
		setLastShader(true);
	}
	createRenderPassAndFramebuffer(tr, shaderState, trl.renderPassAdd, trl.framebufferAdd, trl.framebufferAdd2);

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
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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
	createPipelineLayout(&trl.pipelineLayout);

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
	pipelineInfo.layout = trl.pipelineLayout;
	pipelineInfo.renderPass = trl.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &trl.graphicsPipeline) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
	pipelineInfo.renderPass = trl.renderPassAdd;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &trl.graphicsPipelineAdd) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}

	// create and map vertex buffer in GPU (for lines added for a single frame)
	VkDeviceSize bufferSize = sizeof(Vertex) * MAX_DYNAMIC_LINES;
	createVertexBuffer(tr, trl.vertexBufferAdd, bufferSize, trl.vertexBufferAddMemory);
	createCommandBufferLineAdd(tr);
}

void LineShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}


void LineShader::add(vector<LineDef>& linesToAdd)
{
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
}

void LineShader::addOneTime(vector<LineDef>& linesToAdd, ThreadResources& tr) {
	//auto& lines = getInactiveAppDataSet(user)->oneTimeLines;
	if (linesToAdd.size() == 0)
		return;
	auto& vec = tr.lineResources.verticesAddLines;
	// handle fixed lines:
	for (LineDef& line : linesToAdd) {
		Vertex v1, v2;
		v1.color = line.color;
		v1.pos = line.start;
		v2.color = line.color;
		v2.pos = line.end;
		vec.push_back(v1);
		vec.push_back(v2);
	}
}

void LineShader::initialUpload()
{
	if (!enabled) return;
	// create vertex buffer in CPU mem
	vector<Vertex> all;
	// handle fixed lines:
	for (LineDef& line : lines) {
		Vertex v1, v2;
		v1.color = line.color;
		v1.pos = line.start;
		v2.color = line.color;
		v2.pos = line.end;
		all.push_back(v1);
		all.push_back(v2);
	}

	// if there are no fixed lines we have nothing to do here
	if (all.size() == 0) return;

	// create and copy vertex buffer in GPU
	VkDeviceSize bufferSize = sizeof(Vertex) * all.size();
	global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBuffer, vertexBufferMemory);
}

void LineShader::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
}

void LineShader::createDescriptorSets(ThreadResources& tr)
{
	auto& trl = tr.lineResources;
	VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &trl.descriptorSet) != VK_SUCCESS) {
        Error("failed to allocate descriptor sets!");
    }

    // populate descriptor set:
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = trl.uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    array<VkWriteDescriptorSet, 1> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = trl.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	if (engine->isStereo()) {
		if (vkAllocateDescriptorSets(device, &allocInfo, &trl.descriptorSet2) != VK_SUCCESS) {
			Error("failed to allocate descriptor sets!");
		}
		bufferInfo.buffer = trl.uniformBuffer2;
		descriptorWrites[0].dstSet = trl.descriptorSet2;
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void LineShader::createCommandBuffer(ThreadResources& tr)
{
	auto& trl = tr.lineResources;
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &trl.commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(trl.commandBuffer, "LINE COMMAND BUFFER");
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(trl.commandBuffer, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = trl.renderPass;
	renderPassInfo.framebuffer = trl.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = engine->getBackBufferExtent();

	renderPassInfo.clearValueCount = 0;

	vkCmdBeginRenderPass(trl.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	recordDrawCommand(trl.commandBuffer, tr, vertexBuffer);
	vkCmdEndRenderPass(trl.commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = trl.framebuffer2;
		vkCmdBeginRenderPass(trl.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		recordDrawCommand(trl.commandBuffer, tr, vertexBuffer, true);
		vkCmdEndRenderPass(trl.commandBuffer);
	}
	if (vkEndCommandBuffer(trl.commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void LineShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.lineResources.commandBuffer);
	tr.activeCommandBuffers.push_back(tr.lineResources.commandBufferAdd);
};

void LineShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	auto& trl = tr.lineResources;
	if (vertexBuffer == nullptr) return; // no fixed lines to draw
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// bind descriptor sets:
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.pipelineLayout, 0, 1, &trl.descriptorSet, 0, nullptr);
	} else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.pipelineLayout, 0, 1, &trl.descriptorSet2, 0, nullptr);
	}

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(lines.size() * 2), 1, 0, 0);
}

void LineShader::clearAddLines(ThreadResources& tr)
{
	tr.lineResources.verticesAddLines.clear();
}

void LineShader::createCommandBufferLineAdd(ThreadResources& tr)
{
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.lineResources.commandBufferAdd) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(tr.lineResources.commandBufferAdd, "LINE ADD COMMAND BUFFER");
}

void LineShader::recordDrawCommandAdd(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	auto& trl = tr.lineResources;
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = trl.renderPassAdd;
	renderPassInfo.framebuffer = trl.framebufferAdd;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = engine->getBackBufferExtent();

	renderPassInfo.clearValueCount = 0;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.graphicsPipelineAdd);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// bind descriptor sets:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.pipelineLayout, 0, 1, &trl.descriptorSet, 0, nullptr);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(trl.verticesAddLines.size()), 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = trl.framebufferAdd2;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trl.pipelineLayout, 0, 1, &trl.descriptorSet2, 0, nullptr);
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(trl.verticesAddLines.size()), 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void LineShader::prepareAddLines(ThreadResources& tr)
{
	if (!enabled) return;
	recordDrawCommandAdd(tr.lineResources.commandBufferAdd, tr, tr.lineResources.vertexBufferAdd);
}

void LineShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	auto& trl = tr.lineResources;
	// copy ubo to GPU:
	void* data;
	vkMapMemory(device, trl.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, trl.uniformBufferMemory);
	if (engine->isStereo()) {
		vkMapMemory(device, trl.uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, trl.uniformBufferMemory2);
	}

	// mak
	// copy added lines to GPU:
	VkDeviceSize bufferSize = sizeof(Vertex) * MAX_DYNAMIC_LINES;
	size_t copy_size = trl.verticesAddLines.size() * sizeof(Vertex);
	vkMapMemory(device, trl.vertexBufferAddMemory, 0, bufferSize, 0, &data);
	memcpy(data, trl.verticesAddLines.data(), copy_size);
	vkUnmapMemory(device, trl.vertexBufferAddMemory);
}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	if (!enabled) {
		return;
	}
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void LineShader::destroyThreadResources(ThreadResources& tr)
{
	auto& trl = tr.lineResources;
	vkDestroyFramebuffer(device, trl.framebuffer, nullptr);
	vkDestroyFramebuffer(device, trl.framebufferAdd, nullptr);
	vkDestroyRenderPass(device, trl.renderPass, nullptr);
	vkDestroyRenderPass(device, trl.renderPassAdd, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipeline, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipelineAdd, nullptr);
	vkDestroyPipelineLayout(device, trl.pipelineLayout, nullptr);
	vkDestroyBuffer(device, trl.uniformBuffer, nullptr);
	vkFreeMemory(device, trl.uniformBufferMemory, nullptr);
	vkDestroyBuffer(device, trl.vertexBufferAdd, nullptr);
	vkFreeMemory(device, trl.vertexBufferAddMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, trl.framebuffer2, nullptr);
		vkDestroyFramebuffer(device, trl.framebufferAdd2, nullptr);
		vkDestroyBuffer(device, trl.uniformBuffer2, nullptr);
		vkFreeMemory(device, trl.uniformBufferMemory2, nullptr);
	}
}
