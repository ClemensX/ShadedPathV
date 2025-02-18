#include "mainheader.h"

using namespace std;

void BillboardShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);
	resources.addGeometryShaderStageToMVPBuffer(); // we need to signal use of geometry shader for MVP buffer

	// create shader modules
	vertShaderModule = resources.createShaderModule("billboard.vert.spv");
	engine.util.debugNameObjectShaderModule(vertShaderModule, "Billboard Vert Shader");
	geomShaderModule = resources.createShaderModule("billboard.geom.spv");
	engine.util.debugNameObjectShaderModule(geomShaderModule, "Billboard Geom Shader");
	fragShaderModule = resources.createShaderModule("billboard.frag.spv");
	engine.util.debugNameObjectShaderModule(fragShaderModule, "Billboard Frag Shader");

	// descriptor
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, this, 1);

	// push constants
	pushConstantRanges.push_back(billboardPushConstantRange);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		// global fixed lines (one common vertex buffer)
		BillboardSubShader sub;
		sub.init(this, "BillboardSubShader");
		sub.setVertShaderModule(vertShaderModule);
        sub.setGeomShaderModule(geomShaderModule);
		sub.setFragShaderModule(fragShaderModule);
		sub.setVulkanResources(&resources);
		globalSubShaders.push_back(sub);
	}
}

void BillboardShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	BillboardSubShader& ug = globalSubShaders[tr.frameIndex];
	ug.initSingle(tr, shaderState);
}
void BillboardSubShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	frameResources = &tr;
	// uniform buffers for MVP
	billboardShader->createUniformBuffer(uniformBuffer, sizeof(BillboardShader::UniformBufferObject), uniformBufferMemory);
	engine->util.debugNameObjectBuffer(uniformBuffer, "Billboard Uniform Buffer");
	if (engine->isStereo()) {
		billboardShader->createUniformBuffer(uniformBuffer2, sizeof(BillboardShader::UniformBufferObject), uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(uniformBuffer2, "Billboard Uniform Stereo Buffer");
	}

	VulkanHandoverResources handover{};
	handover.mvpBuffer = uniformBuffer;
	handover.mvpBuffer2 = uniformBuffer2;
	handover.mvpSize = sizeof(BillboardShader::UniformBufferObject);
	handover.imageView = nullptr;
	handover.descriptorSet = &descriptorSet;
	handover.descriptorSet2 = &descriptorSet2;
	vulkanResources->createThreadResources(handover);
	
	billboardShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto geomShaderStageInfo = engine->shaders.createGeometryShaderCreateInfo(geomShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = billboardShader->getBindingDescription();
	auto attribute_desc = billboardShader->getAttributeDescriptions();
	auto vertexInputInfo = billboardShader->createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = billboardShader->createStandardRasterizer();

	// multisampling
	auto multisampling = billboardShader->createStandardMultisampling();

	// standard color blending (disabled), transparent pixels are discarded in fragment shader
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = billboardShader->createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout

	vulkanResources->createPipelineLayout(&pipelineLayout, billboardShader);

	// depth stencil
	auto depthStencil = billboardShader->createStandardDepthStencil();

	// create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(size(shaderStages));
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pNext = nullptr;
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

void BillboardShader::initialUpload()
{
	if (!enabled) return;

	// if there are no fixed billboards we have nothing to do here
	if (billboards.size() == 0) return;

	// create and copy vertex buffer in GPU
	VkDeviceSize bufferSize = sizeof(Vertex) * billboards.size();
	global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, billboards.data(), vertexBuffer, vertexBufferMemory, "BillboardShader Global Buffer");
}

void BillboardShader::createCommandBuffer(FrameResources& tr)
{
	BillboardSubShader& sub = globalSubShaders[tr.frameIndex];
	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void BillboardSubShader::createGlobalCommandBufferAndRenderPass(FrameResources& tr)
{
	vulkanResources->updateDescriptorSets(tr);
	auto& device = engine->globalRendering.device;
	auto& global = engine->globalRendering;
	auto& shaders = engine->shaders;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(commandBuffer, "BILLBOARD COMMAND BUFFER");
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
	recordDrawCommand(commandBuffer, tr, billboardShader->vertexBuffer);
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		recordDrawCommand(commandBuffer, tr, billboardShader->vertexBuffer, true);
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void BillboardSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	if (vertexBuffer == nullptr) return; // no fixed billboards to draw
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	// bind global texture array:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

	// bind descriptor sets:
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	} else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet2, 0, nullptr);
	}

	BillboardPushConstants pushConstants;
	pushConstants.worldSizeOneEdge = engine->getWorld()->getWorldSize().x;
	pushConstants.heightmapTextureIndex = billboardShader->heightmapTextureIndex;
	if (billboardShader->heightmapTextureIndex < 0) Error("BillboardShader: app did not set heightmapTextureIndex");
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushConstants), &pushConstants);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(billboardShader->billboards.size()), 1, 0, 0);
}


void BillboardShader::uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	auto& sub = globalSubShaders[tr.frameIndex];
	sub.uploadToGPU(tr, ubo, ubo2);
}

void BillboardSubShader::uploadToGPU(FrameResources& tr, BillboardShader::UniformBufferObject& ubo, BillboardShader::UniformBufferObject& ubo2) {
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

void BillboardShader::addCurrentCommandBuffer(FrameResources& tr) {
	//tr.activeCommandBuffers.push_back(tr.billboardResources.commandBuffer);
};

void BillboardShader::addCommandBuffers(FrameResources* fr, DrawResult* drawResult) {
	int index = drawResult->getNextFreeCommandBufferIndex();
	auto& sub = globalSubShaders[fr->frameIndex];
	drawResult->commandBuffers[index++] = sub.commandBuffer;
}

void BillboardShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void BillboardShader::add(std::vector<BillboardDef>& billboardsToAdd)
{
	if (billboardsToAdd.size() == 0)
		return;
	// convert direction vector to rotation quaternion:
	for (auto& b : billboardsToAdd) {
		if (b.type == 1) {
			glm::vec3 defaultDir(0.0f, 0.0f, 1.0f); // towards positive z
			glm::quat q = MathHelper::RotationBetweenVectors(defaultDir, b.dir);
			//b.dir = glm::vec4(q.x, q.y, q.z, q.w);
			b.dir = glm::vec4(q.w, q.z, -q.y, q.x);
			//b.dir = glm::vec4(q.x, q.y, q.z, q.w);
		}
	}
	billboards.insert(billboards.end(), billboardsToAdd.begin(), billboardsToAdd.end());
}

void BillboardShader::calcVertsOrigin(std::vector<glm::vec3>& verts)
{
	glm::vec3 v0 = glm::vec3(-0.1, 0, 0);
	glm::vec3 v1 = glm::vec3(0.1, 0, 0);
	glm::vec3 v2 = glm::vec3(0, 0.1, 0);
	verts.push_back(v0);
	verts.push_back(v1);
	verts.push_back(v2);
}

BillboardShader::~BillboardShader()
{
	Log("BillboardShader destructor\n");
	if (!enabled) {
		return;
	}
	for (BillboardSubShader& sub : globalSubShaders) {
		sub.destroy();
	}
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, geomShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void BillboardSubShader::init(BillboardShader* parent, std::string debugName) {
	billboardShader = parent;
	name = debugName;
	engine = billboardShader->engine;
	device = engine->globalRendering.device;
	Log("BillboardSubShader init: " << debugName.c_str() << std::endl);
}

void BillboardShader::destroyThreadResources(FrameResources& tr)
{
}

void BillboardSubShader::destroy()
{
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);
	//vkDestroyBuffer(device, trl.billboardVertexBuffer, nullptr);
	//vkFreeMemory(device, trl.billboardVertexBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, framebuffer2, nullptr);
		vkDestroyBuffer(device, uniformBuffer2, nullptr);
		vkFreeMemory(device, uniformBufferMemory2, nullptr);
	}
}
