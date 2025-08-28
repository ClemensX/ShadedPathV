#include "mainheader.h"

using namespace std;

void PBRShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	resources.setResourceDefinition(&vulkanResourceDefinition);

	// create shader modules
	vertShaderModule = resources.createShaderModule("pbr.vert.spv");
	fragShaderModule = resources.createShaderModule("pbr.frag.spv");
	taskShaderModule = resources.createShaderModule("pbr.task.spv");
	meshShaderModule = resources.createShaderModule("pbr.mesh.spv");

	// descriptor
	resources.createDescriptorSetResources(descriptorSetLayout, descriptorPool, this, 1);
	alignedDynamicUniformBufferSize = global->calcConstantBufferSize(sizeof(DynamicModelUBO));
	//resources.createPipelineLayout(&pipelineLayout, this);

	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		// global fixed lines (one common vertex buffer)
		PBRSubShader sub;
		sub.init(this, "PBRLineSubshader");
		sub.setVertShaderModule(vertShaderModule);
		sub.setFragShaderModule(fragShaderModule);
        sub.setTaskShaderModule(taskShaderModule);
        sub.setMeshShaderModule(meshShaderModule);
		sub.setVulkanResources(&resources);
		globalSubShaders.push_back(sub);
	}

	// global mesh storage:
	VkDeviceSize bufferSize = engine.getMeshStorageSize();
    bufferSize = GlobalRendering::minAlign(bufferSize, 16);
	global->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		meshStorageBuffer, meshStorageBufferMemory, "global mesh storage buffer");
	meshStorageNextFreePos = 0;
}

uint64_t PBRShader::allocateMeshStorage(uint64_t size)
{
    size = GlobalRendering::minAlign(size, 16);
	if (meshStorageNextFreePos + size > engine->getMeshStorageSize()) {
		Error("PBRShader: out of global mesh storage memory. Increase in engine settings.");
	}
    uint64_t ret = meshStorageNextFreePos;
    meshStorageNextFreePos += size;
	return ret;
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
	TextureInfo* tiBrdflut = engine->textureStore.getTexture(engine->textureStore.BRDFLUT_TEXTURE_ID);
	TextureInfo* tiIrradiance = engine->textureStore.getTexture(engine->textureStore.IRRADIANCE_TEXTURE_ID);
	TextureInfo* tiPrefileterdEnv = engine->textureStore.getTexture(engine->textureStore.PREFILTEREDENV_TEXTURE_ID);
	auto& objs = engine->objectStore.getSortedList();
	for (auto obj : objs) {
		//Log(" WorldObject texture count: " << obj->mesh->textureInfos.size() << endl);
		if (obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
            continue;
        }
		PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(fr, obj->objectNum);
        PBRTextureIndexes ind;
        fillTextureIndexesFromMesh(ind, obj->mesh);
		if (ind.baseColor == 9) {
            // 9 10 13 12 11 --> 1 2 5 4 3
            //ind.baseColor = 1; // override for testing
			//ind.metallicRoughness = 2;
			//ind.normal = 5;
			//ind.occlusion = 4;
			//ind.emissive = 3;
		}
        buf->indexes = ind;
		buf->jointcount = 0;
		shaderValuesParams params;
		params.prefilteredCubeMipLevels = tiPrefileterdEnv->vulkanTexture.levelCount;
		params.lightDir = glm::vec4(
			sin(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)),
			sin(glm::radians(lightSource.rotation.y)),
			cos(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)),
			0.0f);
		buf->params = params;
        buf->material = obj->mesh->material;
        buf->material.baseColorTextureSet = ind.baseColor;
        buf->material.physicalDescriptorTextureSet = ind.metallicRoughness;
        buf->material.normalTextureSet = ind.normal;
        buf->material.occlusionTextureSet = ind.occlusion;
        buf->material.emissiveTextureSet = ind.emissive;
		buf->material.brdflut = tiBrdflut->index;
        buf->material.irradiance = tiIrradiance->index;
		buf->material.envcube = tiPrefileterdEnv->index;
		//buf->material.texCoordSets.specularGlossiness = 27;
        // calc and set bounding box
        BoundingBox box;
		obj->getBoundingBox(box);
        buf->boundingBox = box;
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
    if (ubo.camPos.x == -42.0f && ubo.camPos.y == -42.0f && ubo.camPos.z == -42.0f) {
        Error("PBRShader: camera position not set in UBO");
    }
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
    vkDestroyShaderModule(device, taskShaderModule, nullptr);
    vkDestroyShaderModule(device, meshShaderModule, nullptr);
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
    engine->util.debugNameObjectDeviceMemory(uniformBufferMemory, "PBR Memory 1");
    if (engine->isStereo()) {
        pbrShader->createUniformBuffer(uniformBuffer2, sizeof(PBRShader::UniformBufferObject), uniformBufferMemory2);
        engine->util.debugNameObjectBuffer(uniformBuffer2, "PBR UBO 2");
        engine->util.debugNameObjectDeviceMemory(uniformBufferMemory2, "PBR Memory 2");
    }
	// dynamic uniform buffer
	auto bufSize = pbrShader->alignedDynamicUniformBufferSize * engine->getMaxObjects();//pbrShader->MaxObjects;
	pbrShader->createUniformBuffer(dynamicUniformBuffer, bufSize, dynamicUniformBufferMemory);
	engine->util.debugNameObjectBuffer(dynamicUniformBuffer, "PBR dynamic UBO");
	engine->util.debugNameObjectDeviceMemory(dynamicUniformBufferMemory, "PBR dynamic UBO Memory");
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
    auto taskShaderStageInfo = engine->shaders.createTaskShaderCreateInfo(taskShaderModule);
    auto meshShaderStageInfo = engine->shaders.createMeshShaderCreateInfo(meshShaderModule);
	auto fragShaderStageInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	//VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	VkPipelineShaderStageCreateInfo shaderStages[] = { taskShaderStageInfo, meshShaderStageInfo, fragShaderStageInfo };

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
	//rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_NONE;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	//rasterizer.cullMode = VK_DYNAMIC_STATE_CULL_MODE;

	// multisampling
	auto multisampling = pbrShader->createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = pbrShader->createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_CULL_MODE_EXT
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// empty for now...

	// pipeline layout
	vulkanResources->createPipelineLayout(&pipelineLayout, pbrShader);

	// depth stencil
	auto depthStencil = pbrShader->createStandardDepthStencil();

	// create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	uint32_t stageCount = static_cast<uint32_t>(sizeof(shaderStages) / sizeof(shaderStages[0]));
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = stageCount;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState; // Optional
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

void PBRSubShader::createGlobalCommandBufferAndRenderPass(FrameResources& tr, bool update)
{
	if (update) {
		// if update is true, we destroy the old one
		if (commandBuffer != nullptr) {
			vkFreeCommandBuffers(device, tr.commandPool, 1, &commandBuffer);
			commandBuffer = nullptr;
        }
	}
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
		recordDrawCommand(commandBuffer, tr, obj, false, update);
	}
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		for (auto obj : objs) {
			recordDrawCommand(commandBuffer, tr, obj, true, update);
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
        // this gives a nice flat effect for mirror-like objects, but it is not correct, as the camera position is different for each eye.
		// So we comment this line for physical correctness...
		//ubo2.camPos = ubo.camPos; 
		vkMapMemory(device, uniformBufferMemory2, 0, sizeof(ubo2), 0, &data);
		memcpy(data, &ubo2, sizeof(ubo2));
		vkUnmapMemory(device, uniformBufferMemory2);
	}
}

void PBRSubShader::recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& fr, WorldObject* obj, bool isRightEye, bool update)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	// Set the cull mode dynamically
	VkCullModeFlags cullMode = obj->mesh->isDoubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
	vkCmdSetCullMode(commandBuffer, cullMode);
	VkBuffer vertexBuffers[] = { obj->mesh->vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, obj->mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	auto buf = pbrShader->getAccessToModel(fr, obj->objectNum);
	buf->flags = 0; // regular PBR rednering
	if (obj->mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
		buf->flags |= PBRShader::MODEL_RENDER_FLAG_USE_VERTEX_COLORS; // no textures, use vertex colors
	}
	if (obj->mesh->flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
		buf->flags |= PBRShader::MODEL_RENDER_FLAG_USE_VERTEX_COLORS; // no textures, use vertex colors
	}

	// meshlet resources:
	// set meshlet count, not the best code location here, but should do
	if (obj->mesh->outMeshletDesc.size() > 0) {
        buf->meshletsCount = static_cast<uint32_t>(obj->mesh->outMeshletDesc.size());
        // assert that the meshlet count is less than the max task work group count
        assert(buf->meshletsCount < engine->globalRendering.globalDeviceInfo.meshShaderProperties.maxTaskWorkGroupCount[0]);
	}
    // update descriptor set for mesh shader:
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = obj->mesh->meshletDescBuffer; // Your VkBuffer handle
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//write.dstSet = descriptorSet;
	write.dstBinding = 2;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	// more storage buffers:
	VkDescriptorBufferInfo bufferInfo3{};
	bufferInfo3.buffer = obj->mesh->localIndexBuffer;
	bufferInfo3.offset = 0;
	bufferInfo3.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo bufferInfo4{};
	bufferInfo4.buffer = obj->mesh->globalIndexBuffer;
	bufferInfo4.offset = 0;
	bufferInfo4.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo bufferInfo5{};
	bufferInfo5.buffer = obj->mesh->vertexStorageBuffer;
	bufferInfo5.offset = 0;
	bufferInfo5.range = VK_WHOLE_SIZE;

	// 2. Prepare VkWriteDescriptorSet for each new buffer
	VkWriteDescriptorSet write3{};
	write3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write3.dstSet = descriptorSet;
	write3.dstBinding = 3; // Binding index in the shader
	write3.dstArrayElement = 0;
	write3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write3.descriptorCount = 1;
	write3.pBufferInfo = &bufferInfo3;

	VkWriteDescriptorSet write4 = write3;
	write4.dstBinding = 4;
	write4.pBufferInfo = &bufferInfo4;

	VkWriteDescriptorSet write5 = write3;
	write5.dstBinding = 5;
	write5.pBufferInfo = &bufferInfo5;

	// 3. Update the descriptor set
	std::array<VkWriteDescriptorSet, 4> writes = { write, write3, write4, write5 };
	// bind global texture array:
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

	// bind descriptor sets:
	// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
	uint32_t objId = obj->objectNum;
	uint32_t dynamicOffset = static_cast<uint32_t>(objId * pbrShader->alignedDynamicUniformBufferSize);
	if (!isRightEye) {
		// left eye
		writes[0].dstSet = writes[1].dstSet = writes[2].dstSet = writes[3].dstSet = descriptorSet;
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
	}
	else {
		// right eye
		writes[0].dstSet = writes[1].dstSet = writes[2].dstSet = writes[3].dstSet = descriptorSet2;
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet2, 1, &dynamicOffset);
	}
	//if (isRightEye) vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->mesh->indices.size()), 1, 0, 0, 0);
	if (obj->mesh->outMeshletDesc.size() > 0) {
		// groupCountX, groupCountY, groupCountZ: number of workgroups to dispatch
		//vkCmdDrawMeshTasksEXT(commandBuffer, obj->mesh->outMeshletDesc.size(), 1, 1);
		// only 1 workgroup: we implement object culling and LOD object selection in task shader
		vkCmdDrawMeshTasksEXT(commandBuffer, 1, 1, 1);
	}
	else
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

void PBRShader::recreateGlobalCommandBuffers()
{
	for (auto& fi : engine->frameInfos) {
		//globalSubShaders[fi.frameIndex].destroy();
		globalSubShaders[fi.frameIndex].createGlobalCommandBufferAndRenderPass(fi, true);
		globalSubShaders[fi.frameIndex].createGlobalCommandBufferAndRenderPass(fi, true);
		//shaders.createCommandBuffers(fi);
	}
}
