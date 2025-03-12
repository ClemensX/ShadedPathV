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
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, this, 1);
	alignedDynamicUniformBufferSize = global->calcConstantBufferSize(sizeof(DynamicModelUBO));
	//resources.createPipelineLayout(&pipelineLayout, this);

	// push constants
	pushConstantRanges.push_back(pbrPushConstantRange);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		// global fixed lines (one common vertex buffer)
		PBRSubShader sub;
		sub.init(this, "PBRLineSubshader");
		sub.setVertShaderModule(vertShaderModule);
		sub.setFragShaderModule(fragShaderModule);
		sub.setVulkanResources(&resources);
		globalSubShaders.push_back(sub);
	}
}

void PBRShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
	PBRSubShader& ug = globalSubShaders[tr.frameIndex];
	ug.initSingle(tr, shaderState);
    // do not allocate command buffer here, as it is done in createCommandBuffer, after app init() in prepareDrawing()
	//ug.allocateCommandBuffer(tr, &ug.commandBuffer, "PBR PERMANENT COMMAND BUFFER");
}

void PBRShader::initialUpload()
{
	// upload all meshes from store:
	auto& list = engine->meshStore.getSortedList();
	for (auto objptr : list) {
		engine->meshStore.uploadObject(objptr);
	}
}

void PBRShader::fillTextureIndexesFromMesh(PBRTextureIndexes& ind, MeshInfo* mesh)
{
	ind.baseColor = mesh->baseColorTexture ? mesh->baseColorTexture->index : -1;
	ind.metallicRoughness = mesh->metallicRoughnessTexture ? mesh->metallicRoughnessTexture->index : -1;
	ind.normal = mesh->normalTexture ? mesh->normalTexture->index : -1;
	ind.occlusion = mesh->occlusionTexture ? mesh->occlusionTexture->index : -1;
	ind.emissive = mesh->emissiveTexture ? mesh->emissiveTexture->index : -1;
}

void PBRShader::prefillModelParameters(FrameResources& fr)
{
	auto& objs = engine->objectStore.getSortedList();
	for (auto obj : objs) {
		//Log(" WorldObject texture count: " << obj->mesh->textureInfos.size() << endl);
		if (obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
            continue;
        }
		PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(fr, obj->objectNum);
        PBRTextureIndexes ind;
        fillTextureIndexesFromMesh(ind, obj->mesh);
        buf->indexes = ind;
		buf->jointcount = 0;
		shaderValuesParams params;
        buf->params = params;
        buf->material = obj->mesh->material;
        buf->material.baseColorTextureSet = ind.baseColor;
        buf->material.physicalDescriptorTextureSet = ind.metallicRoughness;
        buf->material.normalTextureSet = ind.normal;
        buf->material.occlusionTextureSet = ind.occlusion;
        buf->material.emissiveTextureSet = ind.emissive;
	}

}

void PBRShader::createCommandBuffer(FrameResources& tr)
{
	PBRSubShader& sub = globalSubShaders[tr.frameIndex];
	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void PBRShader::addCommandBuffers(FrameResources* fr, DrawResult* drawResult) {
	int index = drawResult->getNextFreeCommandBufferIndex();
	auto& sub = globalSubShaders[fr->frameIndex];
	drawResult->commandBuffers[index++] = sub.commandBuffer;
}

void PBRShader::uploadToGPU(FrameResources& fr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	auto& sub = globalSubShaders[fr.frameIndex];
    sub.uploadToGPU(fr, ubo, ubo2);
}

PBRShader::DynamicModelUBO* PBRShader::getAccessToModel(FrameResources& fr, UINT num)
{
	auto& sub = globalSubShaders[fr.frameIndex];
	char* c_ptr = static_cast<char*>(sub.dynamicUniformBufferCPUMemory);
	c_ptr += num * alignedDynamicUniformBufferSize;
	return (DynamicModelUBO*)c_ptr;
}

PBRShader::~PBRShader()
{
	Log("PBRShader destructor\n");
	if (!enabled) {
		return;
	}
    for (PBRSubShader& sub : globalSubShaders) {
        sub.destroy();
    }
	//vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

// PBRSubShader

void PBRSubShader::init(PBRShader* parent, std::string debugName) {
    pbrShader = parent;
    name = debugName;
    engine = pbrShader->engine;
    device = engine->globalRendering.device;
    Log("PBRSubShader init: " << debugName.c_str() << std::endl);
}

void PBRSubShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
    frameResources = &tr;
    // MVP uniform buffer
    pbrShader->createUniformBuffer(uniformBuffer, sizeof(PBRShader::UniformBufferObject), uniformBufferMemory);
    engine->util.debugNameObjectBuffer(uniformBuffer, "PBR UBO 1");
    engine->util.debugNameObjectDeviceMmeory(uniformBufferMemory, "PBR Memory 1");
    if (engine->isStereo()) {
        pbrShader->createUniformBuffer(uniformBuffer2, sizeof(PBRShader::UniformBufferObject), uniformBufferMemory2);
        engine->util.debugNameObjectBuffer(uniformBuffer2, "PBR UBO 2");
        engine->util.debugNameObjectDeviceMmeory(uniformBufferMemory2, "PBR Memory 2");
    }
	// dynamic uniform buffer
	auto bufSize = pbrShader->alignedDynamicUniformBufferSize * pbrShader->MaxObjects;
	pbrShader->createUniformBuffer(dynamicUniformBuffer, bufSize, dynamicUniformBufferMemory);
	engine->util.debugNameObjectBuffer(dynamicUniformBuffer, "PBR dynamic UBO");
	engine->util.debugNameObjectDeviceMmeory(dynamicUniformBufferMemory, "PBR dynamic UBO Memory");
	// permanently map the dynamic buffer to CPU memory:
	vkMapMemory(device, dynamicUniformBufferMemory, 0, bufSize, 0, &dynamicUniformBufferCPUMemory);
	void* data = dynamicUniformBufferCPUMemory;
	
	VulkanHandoverResources handover{};
    handover.mvpBuffer = uniformBuffer;
    handover.mvpBuffer2 = uniformBuffer2;
    handover.mvpSize = sizeof(PBRShader::UniformBufferObject);
    handover.imageView = nullptr;
    handover.descriptorSet = &descriptorSet;
    handover.descriptorSet2 = &descriptorSet2;
	handover.dynBuffer = dynamicUniformBuffer;
	handover.dynBufferSize = sizeof(PBRShader::DynamicModelUBO);
	handover.debugBaseName = engine->util.createDebugName("ThreadResources.pbrShader", tr.frameIndex);
    assert(pbrShader->descriptorSetLayout != nullptr);
    assert(pbrShader->descriptorPool != nullptr);
    handover.shader = pbrShader;
    vulkanResources->createThreadResources(handover);

    pbrShader->createRenderPassAndFramebuffer(tr, shaderState, renderPass, framebuffer, framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto binding_desc = pbrShader->getBindingDescription();
	auto attribute_desc = pbrShader->getAttributeDescriptions();
	auto vertexInputInfo = pbrShader->createVertexInputCreateInfo(&binding_desc, attribute_desc.data(), attribute_desc.size());

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkPipelineViewportStateCreateInfo viewportState = shaderState.viewportState;

	// rasterizer
	auto rasterizer = pbrShader->createStandardRasterizer();
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_NONE;

	// multisampling
	auto multisampling = pbrShader->createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = pbrShader->createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	vulkanResources->createPipelineLayout(&pipelineLayout, pbrShader);

	// depth stencil
	auto depthStencil = pbrShader->createStandardDepthStencil();

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
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(pbrShader->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void PBRSubShader::allocateCommandBuffer(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName)
{
	pbrShader->prefillModelParameters(tr);

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
	engine->util.debugNameObjectCommandBuffer(commandBuffer, "PBR COMMAND BUFFER");
}

void PBRSubShader::addRenderPassAndDrawCommands(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer)
{
}

void PBRSubShader::createGlobalCommandBufferAndRenderPass(FrameResources& tr)
{
	allocateCommandBuffer(tr, &commandBuffer, "PBR COMMAND BUFFER");

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
	// add draw commands for all valid objects:
	//auto &objs = engine->meshStore.getSortedList();
	auto& objs = engine->objectStore.getSortedList();
	for (auto obj : objs) {
		recordDrawCommand(commandBuffer, tr, obj);
	}
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		for (auto obj : objs) {
			recordDrawCommand(commandBuffer, tr, obj, true);
		}
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void PBRSubShader::uploadToGPU(FrameResources& tr, PBRShader::UniformBufferObject& ubo, PBRShader::UniformBufferObject& ubo2) {
	// copy ubo to GPU:
	void* data;
	if (true) {
		vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBufferMemory);
	}
	if (engine->isStereo() && true) {
		vkMapMemory(device, uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, uniformBufferMemory2);
	}
}

void PBRSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& fr, WorldObject* obj, bool isRightEye)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	VkBuffer vertexBuffers[] = { obj->mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, obj->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	// bind global texture array:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

	// bind descriptor sets:
	// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
	uint32_t objId = obj->objectNum;
	uint32_t dynamicOffset = static_cast<uint32_t>(objId * pbrShader->alignedDynamicUniformBufferSize);
	if (!isRightEye) {
		// left eye
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
	}
	else {
		// right eye
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet2, 1, &dynamicOffset);
	}
	pbrPushConstants pushConstants;
	pushConstants.mode = obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES) ? 1 : 0;
	//if (isRightEye) pushConstants.mode = 2;
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pbrPushConstants), &pushConstants);
	//if (isRightEye) vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->mesh->indices.size()), 1, 0, 0, 0);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->mesh->indices.size()), 1, 0, 0, 0);
}

void PBRSubShader::destroy()
{
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);
	vkDestroyBuffer(device, dynamicUniformBuffer, nullptr);
	vkFreeMemory(device, dynamicUniformBufferMemory, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(device, framebuffer2, nullptr);
		vkDestroyBuffer(device, uniformBuffer2, nullptr);
		vkFreeMemory(device, uniformBufferMemory2, nullptr);
	}
}
