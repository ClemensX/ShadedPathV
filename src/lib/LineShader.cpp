#include "mainheader.h"

using namespace std;

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	engine.globalUpdate.registerShader(this);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("line.vert.spv");
	fragShaderModule = resources.createShaderModule("line.frag.spv");

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, 3);
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

void LineShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	ug.initSingle(tr, shaderState);
	ug.allocateCommandBuffer(tr, &ug.commandBuffer, "LINE PERMANENT COMMAND BUFFER");
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.initSingle(tr, shaderState);
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * LineShader::MAX_DYNAMIC_LINES;
	createVertexBuffer(pf.vertexBufferLocal, bufferSize, pf.vertexBufferMemoryLocal);
	pf.allocateCommandBuffer(tr, &pf.commandBuffer, "LINE ADD COMMAND BUFFER");
	LineSubShader& sub = globalLineSubShaders[tr.threadResourcesIndex];
	sub.initSingle(tr, shaderState);
}

void LineShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
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
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBufferFixedGlobal, vertexBufferMemoryFixedGlobal, "LineShader Global Buffer");
}

void LineShader::createCommandBuffer(ThreadResources& tr)
{
	LineSubShader& sub = globalLineSubShaders[tr.threadResourcesIndex];
	sub.drawCount = lines.size() * 2;

	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void LineShader::addCurrentCommandBuffer(ThreadResources& tr) {
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	if (ug.active) {
		tr.activeCommandBuffers.push_back(ug.commandBuffer);
	}

	auto& perFrameSubShader = perFrameLineSubShaders[tr.threadResourcesIndex];
	if (perFrameSubShader.drawCount != 0) {
		tr.activeCommandBuffers.push_back(perFrameSubShader.commandBuffer);
	}
	auto& globalSubShader = globalLineSubShaders[tr.threadResourcesIndex];
	if (globalSubShader.drawCount != 0) {
		tr.activeCommandBuffers.push_back(globalSubShader.commandBuffer);
	}
};

void LineShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
}

void LineShader::clearLocalLines(ThreadResources& tr)
{
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.vertices.clear();
}

void LineShader::addFixedGlobalLines(vector<LineDef>& linesToAdd)
{
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;

	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
}

void LineShader::addOneTime(std::vector<LineDef>& linesToAdd, ThreadResources& tr)
{
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
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
void LineShader::addPermament(std::vector<LineDef>& linesToAdd, ThreadResources& tr)
{
	//assert(engine->isUpdateThread() == false);
	if (engine->globalUpdate.isRunning()) {
		//Error("ERROR: trying to add permanent lines while global update is running\n");
		Log("WARNING: trying to add permanent lines while global update is running\n");
		return;
	}
	if (renderThreadUpdateRunning) {
		Log("WARNING: trying to add permanent lines while another threads runs the same (should not happen)\n");
		return;
	}
	renderThreadUpdateRunning = true;
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
	triggerUpdateThread();
	renderThreadUpdateRunning = false;
	//ug.drawCount = ug.vertices.size();
}

void LineShader::prepareAddLines(ThreadResources& tr)
{
	if (!enabled) return;
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.addRenderPassAndDrawCommands(tr, &pf.commandBuffer, pf.vertexBufferLocal);
}

void LineShader::assertUpdateThread() {
	assert(engine->isUpdateThread() == true);
}

// called from user code in drawing thread
void LineShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	LineSubShader& sub = globalLineSubShaders[tr.threadResourcesIndex];
	sub.uploadToGPU(tr, ubo);
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.uploadToGPU(tr, ubo);
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
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	// handle changed update sets:
	auto* applyGlobalUpdateSet = engine->globalUpdate.getChangedGlobalUpdateSet(tr.currentGlobalUpdateElement, ug.updateNumber);
	if (applyGlobalUpdateSet != nullptr) {
		//Log("render thread " << tr.frameIndex << " should apply global update " << applyGlobalUpdateSet->updateNumber << " set " << applyGlobalUpdateSet->to_string() << endl);
		applyGlobalUpdate(ug, tr, applyGlobalUpdateSet);
	}
	// if we have an active update set, we need to upload its MVP matrix to GPU
	if (tr.currentGlobalUpdateElement != nullptr) {
			ug.uploadToGPU(tr, ubo);
	}
}

static GlobalUpdateElement fake2;

void LineShader::triggerUpdateThread() {
	engine->pushUpdate(&fake2);
}

void LineShader::resourceSwitch(GlobalResourceSet set)
{
	if (set == GlobalResourceSet::SET_A) {
		LogCond(LOG_GLOBAL_UPDATE, "LineShader::resourceSwitch() to SET_A" << endl;)
	}
	else Error("not implemented");
}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	if (!enabled) {
		return;
	}
	reuseUpdateElement(&globalUpdateElementA);
	reuseUpdateElement(&globalUpdateElementB);
	for (LineSubShader sub : globalLineSubShaders) {
		sub.destroy();
	}
	for (LineSubShader sub : perFrameLineSubShaders) {
		sub.destroy();
	}
	for (LineSubShader sub : globalUpdateLineSubShaders) {
		sub.destroy();
	}
	vkDestroyBuffer(device, vertexBufferFixedGlobal, nullptr);
	vkFreeMemory(device, vertexBufferMemoryFixedGlobal, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void LineShader::destroyThreadResources(ThreadResources& tr)
{
	if (engine->isStereo()) {
	}
}

// LineSubShader

void LineSubShader::init(LineShader* parent, std::string debugName) {
	lineShader = parent;
	name = debugName;
	engine = lineShader->engine;
	device = &engine->global.device;
	Log("LineSubShader init: " << debugName.c_str() << std::endl);
}

void LineSubShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	VulkanHandoverResources handover;
	// MVP uniform buffer
	lineShader->createUniformBuffer(uniformBuffer, sizeof(LineShader::UniformBufferObject),
		uniformBufferMemory);
	handover.mvpBuffer = uniformBuffer;
	handover.mvpSize = sizeof(LineShader::UniformBufferObject);
	handover.descriptorSet = &descriptorSet;
	vulkanResources->createThreadResources(handover);

	lineShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer);

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
	//pipelineInfo.renderPass = trl.renderPassAdd;
	//if (vkCreateGraphicsPipelines(lineShader->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &trl.graphicsPipelineAdd) != VK_SUCCESS) {
	//	Error("failed to create graphics pipeline!");
	//}

	// create and map vertex buffer in GPU (for lines added for a single frame)
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * LineShader::MAX_DYNAMIC_LINES;
	//lineShader->createVertexBuffer(tr, trl.vertexBufferAdd, bufferSize, trl.vertexBufferAddMemory);
	//createCommandBufferLineAdd(tr);
}

void LineSubShader::allocateCommandBuffer(ThreadResources& tr, VkCommandBuffer* cmdBuferPtr, const char* debugName)
{
	vulkanResources->updateDescriptorSets(tr);
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

void LineSubShader::addRenderPassAndDrawCommands(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer)
{
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
	recordDrawCommand(commandBuffer, tr, vertexBuffer);
	vkCmdEndRenderPass(commandBuffer);
	//if (engine->isStereo()) {
	//	renderPassInfo.framebuffer = trl.framebuffer2;
	//	vkCmdBeginRenderPass(trl.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	//	recordDrawCommand(trl.commandBuffer, tr, vertexBuffer, true);
	//	vkCmdEndRenderPass(trl.commandBuffer);
	//}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void LineSubShader::createGlobalCommandBufferAndRenderPass(ThreadResources& tr)
{
	allocateCommandBuffer(tr, &commandBuffer, "LINE COMMAND BUFFER");
	addRenderPassAndDrawCommands(tr, &commandBuffer, lineShader->vertexBufferFixedGlobal);
}

void LineSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
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
		//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineShader->pipelineLayout, 0, 1, &trl.descriptorSet2, 0, nullptr);
	}

	//vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineShader->lines.size() * 2), 1, 0, 0);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(drawCount), 1, 0, 0);
}

void LineSubShader::uploadToGPU(ThreadResources& tr, LineShader::UniformBufferObject& ubo) {
	if (drawCount == 0) return;
	// copy ubo to GPU:
	auto& device = lineShader->device;
	void* data;
	//globalLineSubShader.uploadToGPU(tr, ubo);
	vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBufferMemory);
	//if (engine->isStereo()) {
	//	vkMapMemory(device, trl.uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
	//	memcpy(data, &ubo2, sizeof(ubo2));
	//	vkUnmapMemory(device, trl.uniformBufferMemory2);
	//}

}

void LineShader::createUpdateSet(GlobalUpdateElement& el)
{
}

void LineSubShader::destroy() {
	vkDestroyPipeline(*device, graphicsPipeline, nullptr);
	vkDestroyFramebuffer(*device, framebuffer, nullptr);
	vkDestroyRenderPass(*device, renderPass, nullptr);
	vkDestroyBuffer(*device, uniformBuffer, nullptr);
	vkFreeMemory(*device, uniformBufferMemory, nullptr);
	vkDestroyBuffer(*device, vertexBufferLocal, nullptr);
	vkFreeMemory(*device, vertexBufferMemoryLocal, nullptr);
}

// called from update thread
void LineShader::updateGlobal(GlobalUpdateElement& currentSet)
{
	assertUpdateThread();
	//Log("LineShader::updateGlobal " << currentSet.updateNumber << " set " << currentSet.to_string() << endl);
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * verticesPermanent.size();
	LineShaderUpdateElement* updateElem = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	if (currentSet.updateDesignator == GlobalUpdateDesignator::SET_A) {
		updateElem = &globalUpdateElementA;
	} else {
		updateElem = &globalUpdateElementB;
	}
	//Log("LineShader::updateGlobal wants to update to gen " << currentSet.updateNumber << " would delete, draw count " << updateElem->drawCount << endl);
	updateElem->active = true;
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, verticesPermanent.data(),
		updateElem->vertexBuffer, updateElem->vertexBufferMemory, "LineShader Global UPDATE Buffer " + currentSet.to_string(), GlobalRendering::QueueSelector::TRANSFER);
	updateElem->drawCount = verticesPermanent.size();
	//this_thread::sleep_for(chrono::milliseconds(3000)); //test slow update
}

void LineShader::reuseUpdateElement(LineShaderUpdateElement* el)
{
	if (el->vertexBuffer == nullptr) return;
	if (el->vertexBufferMemory == nullptr) return;
	//LogCond(LOG_GLOBAL_UPDATE, "reuseUpdateElement:  destroy buffer " << hex << el->vertexBuffer << endl);
	//Log("reuseUpdateElement:  destroy buffer " << hex << el->vertexBuffer << endl);
	vkDestroyBuffer(device, el->vertexBuffer, nullptr);
	el->vertexBuffer = nullptr;
	vkFreeMemory(device, el->vertexBufferMemory, nullptr);
	el->vertexBufferMemory = nullptr;
}

// called from drawing thread
void LineShader::applyGlobalUpdate(LineSubShader& updateSubShader, ThreadResources& tr, GlobalUpdateElement* updateSet)
{
	auto* shaderResources = getMatchingShaderResources(updateSet);
	updateSubShader.drawCount = shaderResources->drawCount;
	updateSubShader.addRenderPassAndDrawCommands(tr, &updateSubShader.commandBuffer, shaderResources->vertexBuffer);
	updateSubShader.active = true;
	updateSubShader.updateNumber = updateSet->updateNumber;
	engine->globalUpdate.markGlobalUpdateSetAsUsed(updateSet, tr);
}

void LineShader::freeUpdateResources(GlobalUpdateElement* updateSet)
{
	auto* shaderResources = getMatchingShaderResources(updateSet);
	reuseUpdateElement(shaderResources);
}