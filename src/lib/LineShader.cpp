#include "mainheader.h"

using namespace std;

void LineShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("line.vert.spv");
	fragShaderModule = resources.createShaderModule("line.frag.spv");

	// descriptor set layout
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);
	resources.createPipelineLayout(&pipelineLayout);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		LineSubShader sub;
		sub.init(this, "GlobalLineSubshader");
		sub.setVertShaderModule(vertShaderModule);
		sub.setFragShaderModule(fragShaderModule);
		//sub.setResources(&globalLineThreadResources);
		sub.setVulkanResources(&resources);
		//sub.initSingle(shaderState);
		lineSubShaders.push_back(sub);
	}
}

void LineShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	LineSubShader& sub = lineSubShaders[tr.threadResourcesIndex];
	//if (!globalLineSubShader.initDone)
		sub.initSingle(tr, shaderState);
}

void LineShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}


void LineShader::initialUpload()
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
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, all.data(), vertexBuffer, vertexBufferMemory);
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
	LineSubShader& sub = lineSubShaders[tr.threadResourcesIndex];
	//if (!globalLineSubShader.commandBufferDone) {
		sub.createCommandBuffer(tr);
	//}
}

void LineShader::addCurrentCommandBuffer(ThreadResources& tr) {
	//tr.activeCommandBuffers.push_back(tr.lineResources.commandBuffer);
	tr.activeCommandBuffers.push_back(lineSubShaders[tr.threadResourcesIndex].commandBuffer);

};

void LineShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
}

void LineShader::clearLocalLines(ThreadResources& tr)
{
}

void LineShader::addGlobalConst(vector<LineDef>& linesToAdd)
{
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;

	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
}

void LineShader::addLocalLines(std::vector<LineDef>& linesToAdd, ThreadResources& tr)
{
}

void LineShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	LineSubShader& sub = lineSubShaders[tr.threadResourcesIndex];
	sub.uploadToGPU(tr, ubo);
}

void LineShader::update(ShaderUpdateElement *el)
{
	LineShaderUpdateElement *u = static_cast<LineShaderUpdateElement*>(el);
	// TODO think about moving update_finished logic to base class
	Log("update line shader global buffer via slot " << u->arrayIndex << " update num " << u->num << endl);
	Log("  --> push " << u->linesToAdd->size() << " lines to GPU" << endl);
	GlobalResourceSet set = getInactiveResourceSet();
	updateAndSwitch(u->linesToAdd, set);
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
	if (set == GlobalResourceSet::SET_A) {
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
}

void LineShader::switchGlobalThreadResources(ThreadResources& res)
{
}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	if (!enabled) {
		return;
	}
	//vkDestroyBuffer(device, vertexBuffer, nullptr);
	//vkFreeMemory(device, vertexBufferMemory, nullptr);
	//globalLineSubShader.destroy();
	for (LineSubShader sub : lineSubShaders) {
		sub.destroy();
	}
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
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
void LineSubShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	LineSubShader& sub = lineShader->lineSubShaders[tr.threadResourcesIndex];
	VulkanHandoverResources handover;
	// MVP uniform buffer
	lineShader->createUniformBuffer(tr, uniformBuffer,
		sizeof(LineShader::UniformBufferObject), uniformBufferMemory);
	handover.mvpBuffer = uniformBuffer;
	handover.mvpSize = sizeof(LineShader::UniformBufferObject);
	handover.descriptorSet = &descriptorSet;
	vulkanResources->createThreadResources(handover);

	lineShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer /*trl.framebuffer2*/);

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
	if (vkCreateGraphicsPipelines(lineShader->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &sub.graphicsPipeline) != VK_SUCCESS) {
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
	initDone = true;
}

void LineSubShader::createCommandBuffer(ThreadResources& tr)
{
	LineSubShader& sub = lineShader->lineSubShaders[tr.threadResourcesIndex];
	vulkanResources->updateDescriptorSets(tr);
	auto& engine = lineShader->engine;
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &sub.commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(sub.commandBuffer, "LINE COMMAND BUFFER");
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(sub.commandBuffer, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = engine->getBackBufferExtent();

	renderPassInfo.clearValueCount = 0;

	vkCmdBeginRenderPass(sub.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	recordDrawCommand(sub.commandBuffer, tr, lineShader->vertexBuffer);
	vkCmdEndRenderPass(sub.commandBuffer);
	//if (engine->isStereo()) {
	//	renderPassInfo.framebuffer = trl.framebuffer2;
	//	vkCmdBeginRenderPass(trl.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	//	recordDrawCommand(trl.commandBuffer, tr, vertexBuffer, true);
	//	vkCmdEndRenderPass(trl.commandBuffer);
	//}
	if (vkEndCommandBuffer(sub.commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
	commandBufferDone = true;
}

void LineSubShader::initialUpload()
{
}

void LineSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	LineSubShader& sub = lineShader->lineSubShaders[tr.threadResourcesIndex];
	if (vertexBuffer == nullptr) return; // no fixed lines to draw
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sub.graphicsPipeline);
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

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineShader->lines.size() * 2), 1, 0, 0);
}

void LineSubShader::uploadToGPU(ThreadResources& tr, LineShader::UniformBufferObject& ubo) {
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
	auto& device = lineShader->device;
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	//vkDestroyFramebuffer(device, trl.framebufferAdd, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	//vkDestroyRenderPass(device, trl.renderPassAdd, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);

}
