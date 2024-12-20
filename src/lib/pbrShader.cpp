#include "mainheader.h"

using namespace std;

void PBRShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("pbr.vert.spv");
	fragShaderModule = resources.createShaderModule("pbr.frag.spv");

	// descriptor
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool);
	alignedDynamicUniformBufferSize = global->calcConstantBufferSize(sizeof(DynamicUniformBufferObject));

	// push constants
	pushConstantRanges.push_back(pbrPushConstantRange);
}

void PBRShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources
	// uniform buffer
	createUniformBuffer(str.uniformBuffer, sizeof(UniformBufferObject), str.uniformBufferMemory);
	engine->util.debugNameObjectBuffer(str.uniformBuffer, "PBR UBO 1");
	engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory, "PBR Memory 1");
	if (engine->isStereo()) {
		createUniformBuffer(str.uniformBuffer2, sizeof(UniformBufferObject), str.uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(str.uniformBuffer2, "PBR UBO 2");
		engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory2, "PBR Memory 2");
	}
	// dynamic uniform buffer
	auto bufSize = alignedDynamicUniformBufferSize * MaxObjects;
	createUniformBuffer(str.dynamicUniformBuffer, bufSize, str.dynamicUniformBufferMemory);
	engine->util.debugNameObjectBuffer(str.dynamicUniformBuffer, "PBR dynamic UBO");
	engine->util.debugNameObjectDeviceMmeory(str.dynamicUniformBufferMemory, "PBR dynamic UBO Memory");
	// permanently map the dynamic buffer to CPU memory:
	vkMapMemory(device, str.dynamicUniformBufferMemory, 0, bufSize, 0, &str.dynamicUniformBufferCPUMemory);
	void* data = str.dynamicUniformBufferCPUMemory;
	//Log("PBR mapped dynamic buffer to address: " << hex << data << endl);
	// test:
	//DynamicUniformBufferObject *d = (DynamicUniformBufferObject*)data;
	//mat4 ident = mat4(1.0f);
	//d->model = ident;
	//data = static_cast<char*>(data) + alignedDynamicUniformBufferSize;
	//Log("PBR mapped dynamic buffer to address: " << hex << data << endl);
	//d = (DynamicUniformBufferObject*)data;
	//ident = mat4(2.0f);
	//d->model = ident;
	VulkanHandoverResources handover;
	handover.mvpBuffer = str.uniformBuffer;
	handover.mvpBuffer2 = str.uniformBuffer2;
	handover.mvpSize = sizeof(UniformBufferObject);
	handover.imageView = nullptr;
	handover.descriptorSet = &str.descriptorSet;
	handover.descriptorSet2 = &str.descriptorSet2;
	handover.dynBuffer = str.dynamicUniformBuffer;
	handover.dynBufferSize = sizeof(DynamicUniformBufferObject);
	handover.debugBaseName = engine->util.createDebugName("ThreadResources.pbrShader", tr);
	resources.createThreadResources(handover);

	//createDescriptorSets(tr);
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);

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
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = createStandardRasterizer();
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_NONE;

	// multisampling
	auto multisampling = createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	resources.createPipelineLayout(&str.pipelineLayout, this);
	//resources.createPipelineLayout(&str.pipelineLayout, descriptorSetLayoutForEachMesh, 1);

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

void PBRShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void PBRShader::initialUpload()
{
	// upload all meshes from store:
	auto& list = engine->meshStore.getSortedList();
	for (auto objptr : list) {
		engine->meshStore.uploadObject(objptr);
	}
}

void PBRShader::prefillTextureIndexes(ThreadResources& tr)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources

	auto& objs = engine->objectStore.getSortedList();
	for (auto obj : objs) {
		//Log(" WorldObject texture count: " << obj->mesh->textureInfos.size() << endl);
		if (obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
            continue;
        }
		uint32_t idx = obj->mesh->baseColorTexture->index;
		PBRShader::DynamicUniformBufferObject* buf = engine->shaders.pbrShader.getAccessToModel(tr, obj->objectNum);
		buf->indexes.baseColor = idx;
	}

}

void PBRShader::createCommandBuffer(ThreadResources& tr)
{
	resources.updateDescriptorSets(tr);
	prefillTextureIndexes(tr);
	auto& str = tr.pbrResources; // shortcut to pbr resources
	auto& device = engine->global.device;
	auto& global = engine->global;
	auto& shaders = engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &str.commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(str.commandBuffer, "PBR COMMAND BUFFER");
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
	// add draw commands for all valid objects:
	//auto &objs = engine->meshStore.getSortedList();
	auto& objs = engine->objectStore.getSortedList();
	for (auto obj : objs) {
		recordDrawCommand(str.commandBuffer, tr, obj);
	}
	vkCmdEndRenderPass(str.commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = str.framebuffer2;
		vkCmdBeginRenderPass(str.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		for (auto obj : objs) {
			recordDrawCommand(str.commandBuffer, tr, obj, true);
		}
		vkCmdEndRenderPass(str.commandBuffer);
	}
	if (vkEndCommandBuffer(str.commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void PBRShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.pbrResources.commandBuffer);
};

void PBRShader::recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, WorldObject* obj, bool isRightEye)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.graphicsPipeline);
	VkBuffer vertexBuffers[] = { obj->mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, obj->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	// bind global texture array:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

	// bind descriptor sets:
	// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
	uint32_t objId = obj->objectNum;
	uint32_t dynamicOffset = static_cast<uint32_t>(objId * alignedDynamicUniformBufferSize);
	if (!isRightEye) {
		// left eye
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet, 1, &dynamicOffset);
	} else {
		// right eye
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet2, 1, &dynamicOffset);
	}
	pbrPushConstants pushConstants;
	pushConstants.mode = obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES) ? 1 : 0;
	//if (isRightEye) pushConstants.mode = 2;
	vkCmdPushConstants(commandBuffer, str.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pbrPushConstants), &pushConstants);
	//if (isRightEye) vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->mesh->indices.size()), 1, 0, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->mesh->indices.size()), 1, 0, 0, 0);
}

void PBRShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	auto& str = tr.pbrResources; // shortcut to pbr resources
	// copy ubo to GPU:
	void* data;
	//memcpy(&ubo2.proj, &ubo.proj, sizeof(ubo.proj)); // left wrong, right ok if commented
	if (true) {
		vkMapMemory(device, str.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, str.uniformBufferMemory);
	}
	if (engine->isStereo() && true) {
		vkMapMemory(device, str.uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, str.uniformBufferMemory2);
	}
}

PBRShader::DynamicUniformBufferObject* PBRShader::getAccessToModel(ThreadResources& tr, UINT num)
{
	char* c_ptr = static_cast<char*>(tr.pbrResources.dynamicUniformBufferCPUMemory);
	c_ptr += num * alignedDynamicUniformBufferSize;
	return (DynamicUniformBufferObject*)c_ptr;
}

PBRShader::~PBRShader()
{
	Log("PBRShader destructor\n");
	if (!enabled) {
		return;
	}
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void PBRShader::destroyThreadResources(ThreadResources& tr)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources
	vkDestroyFramebuffer(device, str.framebuffer, nullptr);
	vkDestroyRenderPass(device, str.renderPass, nullptr);
	vkDestroyPipeline(device, str.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, str.pipelineLayout, nullptr);
	vkDestroyBuffer(device, str.uniformBuffer, nullptr);
	vkFreeMemory(device, str.uniformBufferMemory, nullptr);
	vkDestroyBuffer(device, str.dynamicUniformBuffer, nullptr);
	vkFreeMemory(device, str.dynamicUniformBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, str.framebuffer2, nullptr);
		vkDestroyBuffer(device, str.uniformBuffer2, nullptr);
		vkFreeMemory(device, str.uniformBufferMemory2, nullptr);
	}
}
