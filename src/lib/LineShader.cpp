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
	resources.createPipelineLayout(&pipelineLayout);

	updateElementA.isFirstElement = true;
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
	// TODO remove hack
	bool undoLast = false;
	if (isLastShader()) {
		undoLast = true;
		setLastShader(false);
	}
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	ug.initSingle(tr, shaderState);
	ug.allocateCommandBuffer(tr, &ug.commandBuffer, "LINE PERMANENT COMMAND BUFFER");
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.initSingle(tr, shaderState);
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * LineShader::MAX_DYNAMIC_LINES;
	createVertexBuffer(pf.vertexBufferLocal, bufferSize, pf.vertexBufferMemoryLocal);
	pf.allocateCommandBuffer(tr, &pf.commandBuffer, "LINE ADD COMMAND BUFFER");
	if (undoLast) {
		setLastShader(true);
	}
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
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBufferFixedGlobal, vertexBufferMemoryFixedGlobal);
}

void LineShader::createDescriptorSetLayout()
{
	Error("remove this method from base class!");
}

void LineShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void LineShader::createCommandBuffer(ThreadResources& tr)
{
	LineSubShader& sub = globalLineSubShaders[tr.threadResourcesIndex];
	sub.drawCount = lines.size() * 2;

	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void LineShader::addCurrentCommandBuffer(ThreadResources& tr) {
	//LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	//LineShaderUpdateElement* el = getActiveUpdateElement();
	//if (el != nullptr) {
	//	tr.activeCommandBuffers.push_back(ug.commandBuffer); // TODO null on frame index 1
	//	engine->getThreadGroup().log_current_thread();
	//	int num = el->isFirstElement ? 0 : 1;
	//	LogCond(LOG_GLOBAL_UPDATE, "addCurrentCommandBuffer:  added global update command buffer for update element " << num  << " " << hex << el << " buf " << hex << el->vertexBuffer << endl);
	//}
	
	auto& perFrameSubShader = perFrameLineSubShaders[tr.threadResourcesIndex];
	if (perFrameSubShader.drawCount != 0) {
		tr.activeCommandBuffers.push_back(perFrameSubShader.commandBuffer);
	}
	auto& globalSubShader = globalLineSubShaders[tr.threadResourcesIndex];
	if (globalSubShader.drawCount != 0) {
		tr.activeCommandBuffers.push_back(globalSubShader.commandBuffer);
	}
	//tr.activeCommandBuffers.push_back(perFrameLineSubShaders[tr.threadResourcesIndex].commandBuffer);
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

void LineShader::addPermament(std::vector<LineDef>& linesToAdd, ThreadResources& tr)
{
	assert(engine->isUpdateThread == false);
	if (globalUpdateRunning) {
		Error("ERROR: trying to add permanent lines while global update is running\n");
		return;
	}
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	//auto& lines = getInactiveAppDataSet(user)->oneTimeLines;
	ug.vertices.clear();
	if (linesToAdd.size() == 0)
		return;
	auto& vec = ug.vertices;
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
	ug.drawCount = ug.vertices.size();
}

void LineShader::prepareAddLines(ThreadResources& tr)
{
	if (!enabled) return;
	LineSubShader& pf = perFrameLineSubShaders[tr.threadResourcesIndex];
	pf.addRenderPassAndDrawCommands(tr, &pf.commandBuffer, pf.vertexBufferLocal);
}

void LineShader::assertUpdateThread() {
	assert(engine->isUpdateThread == true);
}

void LineShader::preparePermanentLines(ThreadResources& tr)
{
	if (!enabled) return;
	LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
	LogCond(LOG_GLOBAL_UPDATE, "preparePermanentLines: index " << tr.frameIndex << endl);
	//
	//// initiate global update with ug.vertices, then create render pass and draw command
	//LineShaderUpdateElement* el = lockNextUpdateElement();
	//if (el == nullptr) {
	//	Log("ERROR: race condition - no more update slots for LineShader\n");
	//	return;
	//}
	//LineShaderUpdateElement* elActive = getActiveUpdateElement();
	//if (elActive && (el->isFirstElement == elActive->isFirstElement)) {
	//	Log("ERROR race condition - trying to use active update slot\n");
	//}

	//if (elActive) {
	//	int num = elActive->isFirstElement ? 0 : 1;
	//	LogCond(LOG_GLOBAL_UPDATE, "addCurrentCommandBuffer:  added global update command buffer for update element " << num << " " << hex << elActive << " buf " << hex << elActive->vertexBuffer << endl);
	//}
	//reuseUpdateElement(el);
	//el->verticesAddr = &ug.vertices;
	//triggerUpdateThread();
	////doGlobalUpdate(el, ug, tr);

	// TODO: do in update thread, using direct approach for now
	// delete old buffer
	vkDestroyBuffer(device, ug.vertexBufferLocal, nullptr);
	vkFreeMemory(device, ug.vertexBufferMemoryLocal, nullptr);
	// create new buffer
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * ug.vertices.size();
	//engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, ug.vertices.data(), ug.vertexBufferLocal, ug.vertexBufferMemoryLocal);
	// recreate cmd buffer	
	ug.addRenderPassAndDrawCommands(tr, &ug.commandBuffer, ug.vertexBufferLocal);

}

void LineShader::doGlobalUpdate()
{
	//assertUpdateThread();
	//LogCond(LOG_GLOBAL_UPDATE, "LineShader performing background update\n");
	//LineShaderUpdateElement* el = getCurrentlyWorkedOnUpdateElement();
	//int elNum = el->isFirstElement ? 0 : 1;

	//VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * el->verticesAddr->size();
	//engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, el->verticesAddr->data(), el->vertexBuffer, el->vertexBufferMemory,
	//	GlobalRendering::QueueSelector::TRANSFER, GlobalRendering::QUEUE_FLAG_PERMANENT_UPDATE);
	//LogCond(LOG_GLOBAL_UPDATE, "doGlobalUpdate:  uploaded " << el->verticesAddr->size() << " vertices via new vkbuffer " << hex << el->vertexBuffer  << " for update element " << elNum << endl);
	//engine->getThreadGroup().log_current_thread();
	//// first set update flag in sub shaders, then here:
	//for (int i = 0; i < engine->getFramesInFlight(); i++) {
	//	LineSubShader& ug = globalUpdateLineSubShaders[i];
	//	//ug.handlePermanentUpdate = true;
	//}
	//permanentUpdatePending = true;
	//el->active = true;
	////el->activationFrameNum = tr.frameNum;
}

void LineShader::reuseUpdateElement(LineShaderUpdateElement* el)
{
	LogCond(LOG_GLOBAL_UPDATE, "reuseUpdateElement:  destroy buffer " << hex << el->vertexBuffer << endl);
	vkDestroyBuffer(device, el->vertexBuffer, nullptr);
	vkFreeMemory(device, el->vertexBufferMemory, nullptr);
}

void LineShader::doGlobalUpdate(LineShaderUpdateElement* el, LineSubShader& ug, ThreadResources& tr)
{
	assertUpdateThread();
	// TODO free last gen resources
	VkDeviceSize bufferSize = sizeof(LineShader::Vertex) * el->verticesAddr->size();
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, el->verticesAddr->data(), el->vertexBuffer, el->vertexBufferMemory);
	LogCond(LOG_GLOBAL_UPDATE, "doGlobalUpdate:  uploaded " << el->verticesAddr->size() << " vertices" << endl);
	// first set update flag in sub shaders, then here:
	for (int i = 0; i < engine->getFramesInFlight(); i++) {
		LineSubShader& ug = globalUpdateLineSubShaders[i];
		//ug.handlePermanentUpdate = true;
	}
	permanentUpdatePending = true;
	el->active = true;
	el->activationFrameNum = tr.frameNum;
}


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
	auto activeElement = getActiveUpdateElement();	
	if (activeElement != nullptr) {
		LineSubShader& ug = globalUpdateLineSubShaders[tr.threadResourcesIndex];
		ug.uploadToGPU(tr, ubo);
		//ug.handlePermanentUpdates(ug, tr);
	}
}

void LineShader::update(ShaderUpdateElement *el)
{
	LineShaderUpdateElement *u = static_cast<LineShaderUpdateElement*>(el);
	// TODO think about moving update_finished logic to base class
	LogCond(LOG_GLOBAL_UPDATE, "update line shader global buffer via slot " << u->arrayIndex << " update num " << u->num << endl);
	LogCond(LOG_GLOBAL_UPDATE, "  --> push " << u->linesToAdd->size() << " lines to GPU" << endl);
	GlobalResourceSet set = getInactiveResourceSet();
	updateAndSwitch(u->linesToAdd, set);
	LogCond(LOG_GLOBAL_UPDATE, "update line shader global end " << u->arrayIndex << " update num " << u->num << endl);
}
static ShaderUpdateElement fake;

void LineShader::triggerUpdateThread() {
	//engine->pushUpdate(&fake);
}

void LineShader::updateAndSwitch(std::vector<LineDef>* linesToAdd, GlobalResourceSet set)
{
	if (set == GlobalResourceSet::SET_A) {
	}
	else Error("not implemented");
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
	reuseUpdateElement(&updateElementA);
	reuseUpdateElement(&updateElementB);
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
	//vkDestroyPipeline(device, trl.graphicsPipeline, nullptr);
	//vkDestroyPipeline(device, trl.graphicsPipelineAdd, nullptr);
	//vkDestroyPipelineLayout(device, trl.pipelineLayout, nullptr);
	//vkDestroyBuffer(device, trl.uniformBuffer, nullptr);
	//vkFreeMemory(device, trl.uniformBufferMemory, nullptr);
	//vkDestroyBuffer(device, trl.vertexBufferAdd, nullptr);
	//vkFreeMemory(device, trl.vertexBufferAddMemory, nullptr);
	if (engine->isStereo()) {
		//vkDestroyFramebuffer(device, trl.framebuffer2, nullptr);
		//vkDestroyFramebuffer(device, trl.framebufferAdd2, nullptr);
		//vkDestroyBuffer(device, trl.uniformBuffer2, nullptr);
		//vkFreeMemory(device, trl.uniformBufferMemory2, nullptr);
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

void LineSubShader::destroy() {
	vkDestroyPipeline(*device, graphicsPipeline, nullptr);
	vkDestroyFramebuffer(*device, framebuffer, nullptr);
	vkDestroyRenderPass(*device, renderPass, nullptr);
	vkDestroyBuffer(*device, uniformBuffer, nullptr);
	vkFreeMemory(*device, uniformBufferMemory, nullptr);
	vkDestroyBuffer(*device, vertexBufferLocal, nullptr);
	vkFreeMemory(*device, vertexBufferMemoryLocal, nullptr);
}


// using new global update scheme
void LineShader::createUpdateSet(GlobalUpdateElement& el) {
	if (el.updateDesignator == GlobalUpdateDesignator::SET_A) {
		updateElementA.setDesignator = el.updateDesignator;
		Log("LineShader::createUpdateSet A\n");
	} else {
		updateElementB.setDesignator = el.updateDesignator;
		Log("LineShader::createUpdateSet B\n");
	}
	updateElementA.active = false;
}

LineShader::LineShaderUpdateElement* LineShader::getActiveUpdateElement()
{
	if (updateElementA.active) {
		return &updateElementA;
	}
	else if (updateElementB.active) {
		return &updateElementB;
	}
	else {
		return nullptr;
	}
}

void LineShader::signalGlobalUpdateRunning(bool isRunning)
{
	globalUpdateRunning = isRunning;
}
