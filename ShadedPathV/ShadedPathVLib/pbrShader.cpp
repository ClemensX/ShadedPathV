#include "pch.h"

void PBRShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	engine.files.readFile("pbr_vert.spv", file_buffer_vert, FileCategory::FX);
	engine.files.readFile("pbr_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	vertShaderModule = engine.shaders.createShaderModule(file_buffer_vert);
	fragShaderModule = engine.shaders.createShaderModule(file_buffer_frag);

	// descriptor
	createDescriptorSetLayout();

	// descriptor pool
	// 3 buffers: V,P matrices, line data and dynamic uniform buffer for model matrix
	// line data is global buffer
	vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.resize(4);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[1].descriptorCount = 1;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = 1;

	vector<VkDescriptorPoolSize> poolSizesIndep;
	poolSizesIndep.resize(1);
	poolSizesIndep[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizesIndep[0].descriptorCount = static_cast<uint32_t>(MaxObjects);

	createDescriptorPool(poolSizes, poolSizesIndep, static_cast<uint32_t>(5 + MaxObjects));
}

void PBRShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources
	// uniform buffer
	createUniformBuffer(tr, str.uniformBuffer, sizeof(UniformBufferObject), str.uniformBufferMemory);
	engine->util.debugNameObjectBuffer(str.uniformBuffer, "PBR UBO 1");
	engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory, "PBR Memory 1");
	if (engine->isStereo()) {
		createUniformBuffer(tr, str.uniformBuffer2, sizeof(UniformBufferObject), str.uniformBufferMemory2);
		engine->util.debugNameObjectBuffer(str.uniformBuffer2, "PBR UBO 2");
		engine->util.debugNameObjectDeviceMmeory(str.uniformBufferMemory2, "PBR Memory 2");
	}
	// dynamic uniform buffer
	auto bufSize = alignedDynamicUniformBufferSize * MaxObjects;
	createUniformBuffer(tr, str.dynamicUniformBuffer, bufSize, str.dynamicUniformBufferMemory);
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

	createDescriptorSets(tr);
	createRenderPassAndFramebuffer(tr, shaderState, str.renderPass, str.framebuffer, str.framebuffer2);

	// create shader stage
	auto vertShaderStageInfo = createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderStageInfo = createFragmentShaderCreateInfo(fragShaderModule);
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

	// multisampling
	auto multisampling = createStandardMultisampling();

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	auto colorBlending = createStandardColorBlending(colorBlendAttachment);

	// dynamic state
	// empty for now...

	// pipeline layout
	const std::vector<VkDescriptorSetLayout> setLayouts = {
			descriptorSetLayout, descriptorSetLayoutForEachMesh
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &str.pipelineLayout) != VK_SUCCESS) {
		Error("failed to create pipeline layout!");
	}

	//createPipelineLayout(&str.pipelineLayout);

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

void PBRShader::createDescriptorSetLayout()
{
	alignedDynamicUniformBufferSize = global->calcConstantBufferSize(sizeof(DynamicUniformBufferObject));
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding uboDynamicLayoutBinding{};
	uboDynamicLayoutBinding.binding = 1;
	uboDynamicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uboDynamicLayoutBinding.descriptorCount = 1;
	uboDynamicLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboDynamicLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, uboDynamicLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		Error("failed to create descriptor set layout!");
	}
	engine->util.debugNameObjectDescriptorSetLayout(descriptorSetLayout, "PBR Descriptor Set Layout");

	// create descriptorset layout used for each mesh individually:
	VkDescriptorSetLayoutBinding samplerLayoutBinding0{};
	samplerLayoutBinding0.binding = 0;
	samplerLayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding0.descriptorCount = 1;
	samplerLayoutBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding0.pImmutableSamplers = nullptr; // Optional

	std::array<VkDescriptorSetLayoutBinding, 1> bindingsMesh = { samplerLayoutBinding0 };

	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindingsMesh.size());
	layoutInfo.pBindings = bindingsMesh.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutForEachMesh) != VK_SUCCESS) {
		Error("failed to create descriptor set layout!");
	}
	engine->util.debugNameObjectDescriptorSetLayout(descriptorSetLayoutForEachMesh, "PBR MESH Descriptor Set Layout");
}

void PBRShader::createDescriptorSets(ThreadResources& tr)
{
	auto& str = tr.pbrResources; // shortcut to pbr resources
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	if (vkAllocateDescriptorSets(device, &allocInfo, &str.descriptorSet) != VK_SUCCESS) {
		Error("failed to allocate descriptor sets!");
	}
	engine->util.debugNameObjectDescriptorSet(str.descriptorSet, "PBR Descriptor Set 1");

	array<VkWriteDescriptorSet, 2> descriptorWrites{};
	// populate descriptor set:
	VkDescriptorBufferInfo bufferInfo0{};
	bufferInfo0.buffer = str.uniformBuffer;
	bufferInfo0.offset = 0;
	bufferInfo0.range = sizeof(UniformBufferObject);

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = str.descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo0;

	VkDescriptorBufferInfo bufferInfo1{};
	bufferInfo1.buffer = str.dynamicUniformBuffer;
	bufferInfo1.offset = 0;
	bufferInfo1.range = sizeof(DynamicUniformBufferObject);

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = str.descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo1;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	if (engine->isStereo()) {
		if (vkAllocateDescriptorSets(device, &allocInfo, &str.descriptorSet2) != VK_SUCCESS) {
			Error("failed to allocate descriptor sets!");
		}
		engine->util.debugNameObjectDescriptorSet(str.descriptorSet, "PBR Descriptor Set 2");
		bufferInfo0.buffer = str.uniformBuffer2;
		bufferInfo1.buffer = str.dynamicUniformBuffer; // same as for left eye
		descriptorWrites[0].dstSet = str.descriptorSet2;
		descriptorWrites[1].dstSet = str.descriptorSet2;
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

}

void PBRShader::createPerMeshDescriptors(MeshInfo* mesh)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayoutForEachMesh;
	VkResult res = vkAllocateDescriptorSets(device, &allocInfo, &mesh->descriptorSet);
	if ( res != VK_SUCCESS) {
		Error("failed to allocate descriptor sets!");
	}
	engine->util.debugNameObjectDescriptorSet(mesh->descriptorSet, "Mesh Texture Descriptor Set");
	// Descriptor
	array<VkWriteDescriptorSet, 1> descriptorWrites{};
	// populate descriptor set:
	VkDescriptorImageInfo imageInfo0{};
	imageInfo0.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo0.imageView = mesh->textureInfos[0]->imageView;
	imageInfo0.sampler = global->textureSampler;

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = mesh->descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &imageInfo0;
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void PBRShader::createCommandBuffer(ThreadResources& tr)
{
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
	// mesh texture descriptor set is 2nd in pipeline layout
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 1, 1, &obj->mesh->descriptorSet, 0, nullptr);

	// bind descriptor sets:
	// One dynamic offset per dynamic descriptor to offset into the ubo containing all model matrices
	uint32_t objId = obj->objectNum;
	uint32_t dynamicOffset = static_cast<uint32_t>(objId * alignedDynamicUniformBufferSize);
	if (!isRightEye) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet, 1, &dynamicOffset);
	}
	else {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, str.pipelineLayout, 0, 1, &str.descriptorSet2, 1, &dynamicOffset);
	}
	vkCmdDrawIndexed(commandBuffer, obj->mesh->indices.size(), 1, 0, 0, 0);
}

void PBRShader::uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2) {
	auto& str = tr.pbrResources; // shortcut to pbr resources
	// copy ubo to GPU:
	void* data;
	vkMapMemory(device, str.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, str.uniformBufferMemory);
	if (engine->isStereo()) {
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

void PBRShader::generateBRDFLUT()
{
	auto& global = engine->global;
	auto& texStore = engine->textureStore;
	const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
	const int32_t dim = 512;

	// create entry in texture store:
	FrameBufferAttachment attachment{};
	auto ti = texStore.createTextureSlot(BRDFLUT_TEXTURE_ID);

	//createImageCube(twoD->vulkanTexture.width, twoD->vulkanTexture.height, twoD->vulkanTexture.levelCount, VK_SAMPLE_COUNT_1_BIT, twoD->vulkanTexture.imageFormat, VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	attachment.image, attachment.memory);
	// create image and imageview:
	global.createImage(dim, dim, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, attachment.image, attachment.memory);

	ti->vulkanTexture.deviceMemory = nullptr;
	ti->vulkanTexture.layerCount = 1;
	ti->vulkanTexture.imageFormat = format;
	ti->vulkanTexture.image = attachment.image;
	ti->vulkanTexture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	ti->vulkanTexture.deviceMemory = attachment.memory;
	ti->isKtxCreated = false;
	ti->imageView = global.createImageView(ti->vulkanTexture.image, ti->vulkanTexture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	// Sampler
	VkSamplerCreateInfo samplerCI{};
	samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 1.0f;
	samplerCI.maxAnisotropy = 1.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (vkCreateSampler(device, &samplerCI, nullptr, &brdfSampler) != VK_SUCCESS) {
		Error("failed to create brdflut sampler!");
	}

	ti->available = true;
}

PBRShader::~PBRShader()
{
	Log("PBRShader destructor\n");
	if (!enabled) {
		return;
	}
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayoutForEachMesh, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroySampler(device, brdfSampler, nullptr);
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
