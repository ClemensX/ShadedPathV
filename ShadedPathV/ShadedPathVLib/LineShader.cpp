#include "pch.h"

using namespace std;

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("line_vert.spv");
	fragShaderModule = resources.createShaderModule("line_frag.spv");

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);
	//reserveUpdateSlot(updateArray);
	//reserveUpdateSlot(updateArray);
	//startUpdateThread();
}

void LineShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	VulkanHandoverResources handover;
	auto& trl = tr.lineResources;
	// MVP uniform buffer
	createUniformBuffer(tr, trl.uniformBuffer, sizeof(UniformBufferObject), trl.uniformBufferMemory);
	if (engine->isStereo()) {
		createUniformBuffer(tr, trl.uniformBuffer2, sizeof(UniformBufferObject), trl.uniformBufferMemory2);
	}
	handover.mvpBuffer = trl.uniformBuffer;
	handover.mvpBuffer2 = trl.uniformBuffer2;
	handover.mvpSize = sizeof(UniformBufferObject);
	handover.descriptorSet = &trl.descriptorSet;
	handover.descriptorSet2 = &trl.descriptorSet2;
	resources.createThreadResources(handover);

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
	resources.createPipelineLayout(&trl.pipelineLayout);

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
	Error("remove this method from base class!");
}

void LineShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void LineShader::createCommandBuffer(ThreadResources& tr)
{
	auto& trl = tr.lineResources;
	resources.updateDescriptorSets(tr);
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
	if (true) {
		tr.activeCommandBuffers.push_back(tr.lineResources.commandBufferAdd);
	}
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
	}
	else {
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
	size_t numAddLines = trl.verticesAddLines.size();
	if (numAddLines > 0) {
		if (numAddLines > MAX_DYNAMIC_LINES) {
			Error("LineShader added more dynamic lines than allowed max.");
		}
		size_t copy_size = trl.verticesAddLines.size() * sizeof(Vertex);
		vkMapMemory(device, trl.vertexBufferAddMemory, 0, bufferSize, 0, &data);
		memcpy(data, trl.verticesAddLines.data(), copy_size);
		vkUnmapMemory(device, trl.vertexBufferAddMemory);
	}
}

void LineShader::update(ShaderUpdateElement *el)
{
	LineShaderUpdateElement *u = static_cast<LineShaderUpdateElement*>(el);
	// TODO think about moving update_finished logic to base class
	Log("update line shader global buffer via slot " << u->arrayIndex << " update num " << u->num << endl);
	Log("  --> push " << u->linesToAdd->size() << " lines to GPU" << endl);
	GlobalResourceSet set = getInactiveResourceSet();
	updateAndSwitch(u->linesToAdd, set);

	//if (shaderUpdateQueueInfo.update_finished_counter == 0) {
	//	// all previous updates finished: we can handle a new one
	//	shaderUpdateQueueInfo.update_available = 0;
	//}
	//if (shaderUpdateQueueInfo.update_available > 0) {
	//	// race condition: we have an update while another still ist handled by render threads
	//	Log("WARNING RACE CONDITION LineShader - update delayed by 500ms" << endl);
	//	this_thread::sleep_for(chrono::milliseconds(500));
	//	if (shaderUpdateQueueInfo.update_finished_counter == 0) {
	//		// all previous updates finished: we can handle a new one
	//		shaderUpdateQueueInfo.update_available = 0;
	//	}
	//	if (shaderUpdateQueueInfo.update_available > 0) {
	//		// give up
	//		Error("ERROR RACE CONDITION LineShader");
	//	}
	//}
	this_thread::sleep_for(chrono::milliseconds(4000));
	//updateAndSwitch(u.linesToAdd);

	//// after update signal newest generation in shader:
	//shaderUpdateQueueInfo.update_available = u.num;
	//shaderUpdateQueueInfo.update_finished_counter = engine->getFramesInFlight();
	Log("update line shader global end " << u->arrayIndex << " update num " << u->num << endl);
}

void LineShader::updateGlobal(std::vector<LineDef>& linesToAdd)
{
	//Log("LineShader update global start");
	//engine->printUpdateArray(updateArray);
	int i = (int)engine->reserveUpdateSlot(updateArray);
	updateArray[i].shaderInstance = this; // TODO move elsewhere - to shader init?
	updateArray[i].arrayIndex = i; // TODO move elsewhere - to shader init?
	updateArray[i].linesToAdd = &linesToAdd;
	//engine->shaderUpdateQueue.push(i);
	engine->pushUpdate(&updateArray[i]);
	//Log("LineShader update end         ");
	//engine->printUpdateArray(updateArray);
}

void LineShader::updateAndSwitch(std::vector<LineDef>* linesToAdd, GlobalResourceSet set)
{
	// create vertex buffer in CPU mem
	vector<Vertex> all(linesToAdd->size() * 2);
	// handle fixed lines:
	size_t index = 0;
	for (LineDef& line : *linesToAdd) {
		Vertex v1, v2;
		v1.color = line.color;
		v1.pos = line.start;
		v2.color = line.color;
		v2.pos = line.end;
		all[index++] = v1;
		all[index++] = v2;
	}

	// if there are no fixed lines we have nothing to do here
	if (all.size() == 0) return;

	// create and copy vertex buffer in GPU
	VkDeviceSize bufferSize = sizeof(Vertex) * all.size();
	if (set == GlobalResourceSet::SET_A) {
		global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBufferUpdates, vertexBufferMemoryUpdates, GlobalRendering::QueueSelector::TRANSFER);
	}
	else Error("not implemented");
}

void LineShader::resourceSwitch(GlobalResourceSet set)
{
	if (set == GlobalResourceSet::SET_A) {
		Log("LineShader::resourceSwitch() to SET_A" << endl;)
	}
	else Error("not implemented");
}

void LineShader::handleUpdatedResources(ThreadResources& tr)
{
	//if (shaderUpdateQueueInfo.update_available > 0 && tr.global_update_num < shaderUpdateQueueInfo.update_available) {
	//	// not yet switched for this update

	//	// do the switch
	//	switchGlobalThreadResources(tr);

	//	tr.global_update_num = shaderUpdateQueueInfo.update_available;
	//	shaderUpdateQueueInfo.update_finished_counter--;
	//}
}

void LineShader::switchGlobalThreadResources(ThreadResources& res)
{
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	auto& trl = res.lineResources;
	assert(vertexBufferUpdates != vertexBuffer);
	if (vertexBufferUpdates != nullptr) {
		// delete old resources TODO should be done elswhere: we do nor know here when access is finished
		//assert(false);
	}
	if (trl.commandBufferUpdate == nullptr) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = res.commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)1;

		if (vkAllocateCommandBuffers(device, &allocInfo, &trl.commandBufferUpdate) != VK_SUCCESS) {
			Error("failed to allocate command buffers!");
		}
		engine->util.debugNameObjectCommandBuffer(trl.commandBufferUpdate, "GLOBAL UPDATE COMMAND BUFFER");
	}
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
