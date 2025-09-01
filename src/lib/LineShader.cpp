#include "mainheader.h"

using namespace std;

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	//engine.globalUpdate.registerShader(this);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("line.vert.spv");
	fragShaderModule = resources.createShaderModule("line.frag.spv");

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, this, 3); // allocate for 3 subshaders
	resources.createPipelineLayout(&pipelineLayout, this);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		// global fixed lines (one common vertex buffer)
		LineSubShader sub;
		sub.init(this, "GlobalLineSubshader");
		sub.setVertShaderModule(vertShaderModule);
		sub.setFragShaderModule(fragShaderModule);
		sub.setVulkanResources(&resources);
		globalLineSubShaders.push_back(sub);

		// per frame lines 
		LineSubShader pf;
		pf.init(this, "PerFrameLineSubshader");
		pf.setVertShaderModule(vertShaderModule);
		pf.setFragShaderModule(fragShaderModule);
		pf.setVulkanResources(&resources);
		perFrameLineSubShaders.push_back(pf);

		// lines added permanently through global update thread
		LineSubShader gu;
		gu.init(this, "GlobalUpdateLineSubshader");
		gu.setVertShaderModule(vertShaderModule);
		gu.setFragShaderModule(fragShaderModule);
		gu.setVulkanResources(&resources);
		globalUpdateLineSubShaders.push_back(gu);
	}
}

void LineShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	LineSubShader& ug = globalUpdateLineSubShaders[tr.frameIndex];
	ug.initSingle(tr, shaderState);
	ug.allocateCommandBuffer(tr, &ug.commandBuffer, "LINE PERMANENT COMMAND BUFFER");
	LineSubShader& pf = perFrameLineSubShaders[tr.frameIndex];
	pf.initSingle(tr, shaderState);
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * LineShader::MAX_DYNAMIC_LINES;
	createVertexBuffer(pf.vertexBufferLocal, bufferSize, pf.vertexBufferMemoryLocal);
    auto name = engine->util.createDebugName("LINE ADD COMMAND BUFFER", tr.frameIndex);
	pf.allocateCommandBuffer(tr, &pf.commandBuffer, name.c_str());
	LineSubShader& sub = globalLineSubShaders[tr.frameIndex];
	sub.initSingle(tr, shaderState);
}

void LineShader::uploadFixedGlobalLines()
{
	if (!enabled) return;
	//lineSubShaders[0].initialUpload(); // TODO hack
	// create vertex buffer in CPU mem
	vector<LineShader::Vertex> all;
	// handle fixed lines:
	for (LineDef& line : lines) {
		LineShader::Vertex v1, v2;
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
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * all.size();
	engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBufferFixedGlobal, vertexBufferMemoryFixedGlobal, "LineShader Global Buffer");
}

void LineShader::createCommandBuffer(FrameResources& tr)
{
	LineSubShader& sub = globalLineSubShaders[tr.frameIndex];
	sub.drawCount = lines.size() * 2;

	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void LineShader::addCommandBuffers(FrameResources* fr, DrawResult* drawResult) {
	if (!enabled) return;
	int index = drawResult->getNextFreeCommandBufferIndex();
	auto& perFrameSubShader = perFrameLineSubShaders[fr->frameIndex];
	if (perFrameSubShader.drawCount != 0) {
        drawResult->commandBuffers[index++] = perFrameSubShader.commandBuffer;
	}
	auto& globalSubShader = globalLineSubShaders[fr->frameIndex];
    if (globalSubShader.drawCount != 0) {
        drawResult->commandBuffers[index++] = globalSubShader.commandBuffer;
	}
	LineSubShader& ug = globalUpdateLineSubShaders[fr->frameIndex];
	if (ug.active) {
        drawResult->commandBuffers[index++] = ug.commandBuffer;
	}

}


void LineShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
}

void LineShader::clearLocalLines(FrameResources& tr)
{
	if (!enabled) return;
	LineSubShader& pf = perFrameLineSubShaders[tr.frameIndex];
	pf.vertices.clear();
	pf.drawCount = 0;
}

void LineShader::addFixedGlobalLines(vector<LineDef>& linesToAdd)
{
	if (!enabled) return;
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;

	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
}

void LineShader::addOneTime(std::vector<LineDef>& linesToAdd, FrameResources& tr)
{
	if (!enabled) return;
	LineSubShader& pf = perFrameLineSubShaders[tr.frameIndex];
	//auto& lines = getInactiveAppDataSet(user)->oneTimeLines;
	if (linesToAdd.size() == 0)
		return;
	auto& vec = pf.vertices;
	// handle added lines:
	for (LineDef& line : linesToAdd) {
		Vertex v1, v2;
		v1.color = line.color;
		v1.pos = line.start;
		v2.color = line.color;
		v2.pos = line.end;
		vec.push_back(v1);
		vec.push_back(v2);
	}
	pf.drawCount = pf.vertices.size();
}

// called from user code in drawing thread 
void LineShader::addPermament(std::vector<LineDef>& linesToAdd)
{
	verticesPermanent.clear();
	if (linesToAdd.size() == 0)
		return;
	// handle added lines:
	for (LineDef& line : linesToAdd) {
		Vertex v1, v2;
		v1.color = line.color;
		v1.pos = line.start;
		v2.color = line.color;
		v2.pos = line.end;
		verticesPermanent.push_back(v1);
		verticesPermanent.push_back(v2);
	}

	// transfer to GPU
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * verticesPermanent.size();
	VkDeviceMemory vertexBufferMemory = nullptr;
	//Log("LineShader::updateGlobal wants to update to gen " << currentSet.updateNumber << " would delete, draw count " << updateElem->drawCount << endl);
	updateElement.active = true;
	engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, verticesPermanent.data(),
		updateElement.vertexBuffer, updateElement.vertexBufferMemory, "LineShader Global UPDATE Buffer ", GlobalRendering::QueueSelector::TRANSFER);
	updateElement.drawCount = verticesPermanent.size();
	oldUpdateInUse = 2;
}

void LineShader::prepareAddLines(FrameResources& tr)
{
	if (!enabled) return;
	LineSubShader& pf = perFrameLineSubShaders[tr.frameIndex];
	pf.addRenderPassAndDrawCommands(tr, &pf.commandBuffer, pf.vertexBufferLocal);
}

// called from user code in drawing thread
void LineShader::uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	LineSubShader& sub = globalLineSubShaders[tr.frameIndex];
	sub.uploadToGPU(tr, ubo, ubo2);
	LineSubShader& pf = perFrameLineSubShaders[tr.frameIndex];
	pf.uploadToGPU(tr, ubo, ubo2);
	// copy added lines to GPU:
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * LineShader::MAX_DYNAMIC_LINES;
	if (pf.drawCount > 0) {
		void* data;
		if (pf.drawCount > LineShader::MAX_DYNAMIC_LINES) {
			Error("LineShader added more dynamic lines than allowed max.");
		}
		size_t copy_size = pf.drawCount * sizeof(LineShader::Vertex);
		vkMapMemory(device, pf.vertexBufferMemoryLocal, 0, bufferSize, 0, &data);
		memcpy(data, pf.vertices.data(), copy_size);
		vkUnmapMemory(device, pf.vertexBufferMemoryLocal);
	}
	LineSubShader& ug = globalUpdateLineSubShaders[tr.frameIndex];
    ug.uploadToGPU(tr, ubo, ubo2);
}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	if (!enabled) {
		return;
	}
	for (LineSubShader sub : globalLineSubShaders) {
		sub.destroy();
	}
	for (LineSubShader sub : perFrameLineSubShaders) {
		sub.destroy();
	}
	for (LineSubShader sub : globalUpdateLineSubShaders) {
		sub.destroy();
	}
	// destroy update element only once:
    auto& sub = globalUpdateLineSubShaders[0];
	if (sub.activeUpdateElement.vertexBuffer != nullptr) {
		vkDestroyBuffer(device, sub.activeUpdateElement.vertexBuffer, nullptr);
		vkFreeMemory(device, sub.activeUpdateElement.vertexBufferMemory, nullptr);
	}
	vkDestroyBuffer(device, vertexBufferFixedGlobal, nullptr);
	vkFreeMemory(device, vertexBufferMemoryFixedGlobal, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

// LineSubShader

void LineSubShader::init(LineShader* parent, std::string debugName) {
	lineShader = parent;
	name = debugName;
	engine = lineShader->engine;
	device = &engine->globalRendering.device;
	Log("LineSubShader init: " << debugName.c_str() << std::endl);
}

void LineSubShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	frameResources = &tr;
	// MVP uniform buffer
	lineShader->createUniformBuffer(uniformBuffer, sizeof(LineShader::UniformBufferObject),
		uniformBufferMemory);
	engine->util.debugNameObjectBuffer(uniformBuffer, "Line UBO 1");
	engine->util.debugNameObjectDeviceMemory(uniformBufferMemory, "Line Memory 1");
	if (engine->isStereo()) {
		lineShader->createUniformBuffer(uniformBuffer2, sizeof(LineShader::UniformBufferObject), uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(uniformBuffer2, "Line UBO 2");
		engine->util.debugNameObjectDeviceMemory(uniformBufferMemory2, "Line Memory 2");
	}
	VulkanHandoverResources handover;
	handover.mvpBuffer = uniformBuffer;
	handover.mvpBuffer2 = uniformBuffer2;
	handover.mvpSize = sizeof(LineShader::UniformBufferObject);
	handover.imageView = nullptr;
	handover.descriptorSet = &descriptorSet;
	handover.descriptorSet2 = &descriptorSet2;
	handover.dynBuffer = nullptr;
	handover.dynBufferSize = 0;
	handover.debugBaseName = engine->util.createDebugName("ThreadResources.lineShader", tr.frameIndex);
    assert(lineShader->descriptorSetLayout != nullptr);
    assert(lineShader->descriptorPool != nullptr);
	handover.shader = lineShader;
	vulkanResources->createThreadResources(handover);

	lineShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = lineShader->engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = lineShader->engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = lineShader->getBindingDescription();
	auto attribute_desc = lineShader->getAttributeDescriptions();
	auto vertexInputInfo = lineShader->createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = lineShader->createStandardRasterizer();

	// multisampling
	auto multisampling = lineShader->createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = lineShader->createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	//vulkanResources->createPipelineLayout(&trl.pipelineLayout);

	// depth stencil
	auto depthStencil = lineShader->createStandardDepthStencil();

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
	pipelineInfo.layout = lineShader->pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(lineShader->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void LineSubShader::allocateCommandBuffer(FrameResources& tr, VkCommandBuffer* cmdBuferPtr, const char* debugName)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(commandBuffer, debugName);
}

void LineSubShader::addRenderPassAndDrawCommands(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	//Log("LineSubShader begin cmd buffer: " << commandBuffer << endl);
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
	recordDrawCommand(commandBuffer, tr, vertexBuffer);
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		recordDrawCommand(commandBuffer, tr, vertexBuffer, true);
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void LineSubShader::createGlobalCommandBufferAndRenderPass(FrameResources& tr)
{
	allocateCommandBuffer(tr, &commandBuffer, "LINE COMMAND BUFFER");
	addRenderPassAndDrawCommands(tr, &commandBuffer, lineShader->vertexBufferFixedGlobal);
}

void LineSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	if (vertexBuffer == nullptr) return; // no fixed lines to draw
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	// bind descriptor sets:
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineShader->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	}
	else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineShader->pipelineLayout, 0, 1, &descriptorSet2, 0, nullptr);
	}

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(drawCount), 1, 0, 0);
}

void LineSubShader::uploadToGPU(FrameResources& tr, LineShader::UniformBufferObject& ubo, LineShader::UniformBufferObject& ubo2) {
	if (drawCount == 0) return;
	// copy ubo to GPU:
	auto& device = lineShader->device;
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

void LineSubShader::destroy() {
	vkDestroyPipeline(*device, graphicsPipeline, nullptr);
	vkDestroyFramebuffer(*device, framebuffer, nullptr);
	vkDestroyFramebuffer(*device, framebuffer2, nullptr);
	vkDestroyRenderPass(*device, renderPass, nullptr);
	vkDestroyBuffer(*device, uniformBuffer, nullptr);
	vkDestroyBuffer(*device, uniformBuffer2, nullptr);
    //vkFreeCommandBuffers(*device, frameResources->commandPool, 1, &commandBuffer);
	vkFreeMemory(*device, uniformBufferMemory, nullptr);
	vkFreeMemory(*device, uniformBufferMemory2, nullptr);
	vkDestroyBuffer(*device, vertexBufferLocal, nullptr);
	vkFreeMemory(*device, vertexBufferMemoryLocal, nullptr);
}

void LineShader::applyGlobalUpdate(FrameResources& tr)
{
    int index = tr.frameIndex;
	//Log("update frame res " << index << endl);
	auto& sub = globalUpdateLineSubShaders[index];
	oldUpdateInUse--;
	// cleanup old:
	if (sub.active && oldUpdateInUse == 0) {
		vkDestroyBuffer(device, sub.activeUpdateElement.vertexBuffer, nullptr);
        sub.activeUpdateElement.vertexBuffer = nullptr;
		vkFreeMemory(device, sub.activeUpdateElement.vertexBufferMemory, nullptr);
        sub.activeUpdateElement.vertexBufferMemory = nullptr;
	}
	// copy update element so sub shader element:
	sub.activeUpdateElement = updateElement;
	sub.drawCount = updateElement.drawCount;
	sub.addRenderPassAndDrawCommands(tr, &sub.commandBuffer, sub.activeUpdateElement.vertexBuffer);
    sub.active = true;
}
