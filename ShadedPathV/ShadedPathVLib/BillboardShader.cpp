#include "pch.h"

using namespace std;

void BillboardShader::init(ShadedPathEngine& engine, ShaderState &shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);
	resources.addGeometryShaderStageToMVPBuffer(); // we need to signal use of geometry shader for MVP buffer

	// create shader modules
	vertShaderModule = resources.createShaderModule("billboard_vert.spv");
	engine.util.debugNameObjectShaderModule(vertShaderModule, "Billboard Vert Shader");
	geomShaderModule = resources.createShaderModule("billboard_geom.spv");
	engine.util.debugNameObjectShaderModule(geomShaderModule, "Billboard Geom Shader");
	fragShaderModule = resources.createShaderModule("billboard_frag.spv");
	engine.util.debugNameObjectShaderModule(fragShaderModule, "Billboard Frag Shader");

	// descriptor
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);
}

void BillboardShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	auto& str = tr.billboardResources; // shortcut to shader thread resources
	// uniform buffers for MVP
	createUniformBuffer(tr, str.uniformBuffer, sizeof(UniformBufferObject), str.uniformBufferMemory);
	engine->util.debugNameObjectBuffer(str.uniformBuffer, "Billboard Uniform Buffer");
	if (engine->isStereo()) {
		createUniformBuffer(tr, str.uniformBuffer2, sizeof(UniformBufferObject), str.uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(str.uniformBuffer, "Billboard Uniform Stereo Buffer");
	}

	VulkanHandoverResources handover;
	handover.mvpBuffer = str.uniformBuffer;
	handover.mvpBuffer2 = str.uniformBuffer2;
	handover.mvpSize = sizeof(UniformBufferObject);
	handover.imageView = nullptr;
	handover.descriptorSet = &str.descriptorSet;
	handover.descriptorSet2 = &str.descriptorSet2;
	resources.createThreadResources(handover);
	//createDescriptorSets(tr);
	// TODO remove hack
	bool undoLast = false;
	if (isLastShader()) {
		undoLast = true;
		setLastShader(false);
	}
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);
	if (undoLast) {
		setLastShader(true);
	}

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto geomShaderStageInfo = engine->shaders.createGeometryShaderCreateInfo(geomShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = getBindingDescription();
	auto attribute_desc = getAttributeDescriptions();
	auto vertexInputInfo = createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
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
	//createPipelineLayout(&str.pipelineLayout);

	// depth stencil
	auto depthStencil = createStandardDepthStencil();

	// create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = size(shaderStages);
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
	pipelineInfo.layout = str.pipelineLayout;
	pipelineInfo.renderPass = str.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &str.graphicsPipeline) != VK_SUCCESS) {
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
	global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, billboards.data(), vertexBuffer, vertexBufferMemory);
}

// assigning stage flags is a little more advanced because we need to pass some bindings to geom stages
void BillboardShader::createDescriptorSetLayout()
{
	Error("remove this method from base class!");
}

void BillboardShader::createDescriptorSets(ThreadResources& tr)
{
	Error("remove this method from base class!");
}

void BillboardShader::createCommandBuffer(ThreadResources& tr)
{
	resources.updateDescriptorSets(tr);
	auto& trl = tr.billboardResources;
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
	engine->util.debugNameObjectCommandBuffer(trl.commandBuffer, "BILLBOARD COMMAND BUFFER");
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

void BillboardShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye)
{
	auto& str = tr.billboardResources;
	if (vertexBuffer == nullptr) return; // no fixed billboards to draw
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.graphicsPipeline);
	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	// bind global texture array:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

	// bind descriptor sets:
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet, 0, nullptr);
	} else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet2, 0, nullptr);
	}

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(billboards.size()), 1, 0, 0);
}


void BillboardShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	if (!enabled) return;
	auto& trl = tr.billboardResources;
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

}

void BillboardShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.billboardResources.commandBuffer);
};

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
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, geomShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void BillboardShader::destroyThreadResources(ThreadResources& tr)
{
	auto& trl = tr.billboardResources;
	vkDestroyFramebuffer(device, trl.framebuffer, nullptr);
	vkDestroyRenderPass(device, trl.renderPass, nullptr);
	vkDestroyPipeline(device, trl.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, trl.pipelineLayout, nullptr);
	vkDestroyBuffer(device, trl.uniformBuffer, nullptr);
	vkFreeMemory(device, trl.uniformBufferMemory, nullptr);
	//vkDestroyBuffer(device, trl.billboardVertexBuffer, nullptr);
	//vkFreeMemory(device, trl.billboardVertexBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, trl.framebuffer2, nullptr);
		vkDestroyBuffer(device, trl.uniformBuffer2, nullptr);
		vkFreeMemory(device, trl.uniformBufferMemory2, nullptr);
	}
}
