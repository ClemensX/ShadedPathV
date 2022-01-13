#include "pch.h"

VkShaderModule Shaders::createShaderModule(const vector<byte>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(engine.global.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		Error("failed to create shader module!");
	}
	return shaderModule;
}

// SHADER Triangle

void Shaders::initiateShader_Triangle()
{
	enabledTriangle = true;
	simpleShader.init(engine);
	// initialization of globals like shader code
	//  
	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	engine.files.readFile("triangle_vert.spv", file_buffer_vert, FileCategory::FX);
	engine.files.readFile("triangle_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	vertShaderModuleTriangle = createShaderModule(file_buffer_vert);
	fragShaderModuleTriangle = createShaderModule(file_buffer_frag);

	ThemedTimer::getInstance()->start(TIMER_PART_BUFFER_COPY);
	// create vertex buffer
	VkDeviceSize bufferSize = sizeof(simpleShader.vertices[0]) * simpleShader.vertices.size();
	engine.global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, simpleShader.vertices.data(), vertexBufferTriangle, vertexBufferMemoryTriangle);
	ThemedTimer::getInstance()->stop(TIMER_PART_BUFFER_COPY);

	// create index buffer
	bufferSize = sizeof(simpleShader.indices[0]) * simpleShader.indices.size();
	engine.global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bufferSize, simpleShader.indices.data(), indexBufferTriangle, indexBufferMemoryTriangle);

	// descriptor
	simpleShader.createDescriptorSetLayout();

	// load texture
	engine.textureStore.loadTexture("debug.ktx", "debugTexture");
	simpleShader.texture = engine.textureStore.getTexture("debugTexture");

	// pipelines must be created for every rendering thread
	for (auto &res : engine.threadResources) {
		initiateShader_TriangleSingle(res);
	}

}

void Shaders::initiateShader_TriangleSingle(ThreadResources& res)
{
	// uniform buffer
	simpleShader.createUniformBuffer(res);
	simpleShader.createDescriptorSets(res);
	// create shader stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModuleTriangle;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModuleTriangle;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex input
	auto bindingDescription = SimpleShader::getBindingDescription();
	auto attributeDescriptions = SimpleShader::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t> (attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)engine.getBackBufferExtent().width;
	viewport.height = (float)engine.getBackBufferExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = engine.getBackBufferExtent();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// depth and stencil testing
	// empty for now...

	// color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// dynamic state
	// empty for now...

	// pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &simpleShader.descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(engine.global.device, &pipelineLayoutInfo, nullptr, &res.pipelineLayoutTriangle) != VK_SUCCESS) {
		Error("failed to create pipeline layout!");
	}

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
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = res.pipelineLayoutTriangle;
	pipelineInfo.renderPass = res.renderPassInit;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(engine.global.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &res.graphicsPipelineTriangle) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}

}

void Shaders::createCommandBufferTriangle(ThreadResources& tr)
{
	if (!enabledTriangle) return;
	auto& device = engine.global.device;
	auto& global = engine.global;
	auto& shaders = engine.shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferTriangle) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(tr.commandBufferTriangle, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = tr.renderPassInit;
	renderPassInfo.framebuffer = tr.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = engine.getBackBufferExtent();
	VkClearValue clearColor = { {{0.1f, 0.1f, 0.1f, 0.5f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(tr.commandBufferTriangle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	simpleShader.recordDrawCommand(tr.commandBufferTriangle, tr, vertexBufferTriangle, indexBufferTriangle);
	vkCmdEndRenderPass(tr.commandBufferTriangle);
	if (vkEndCommandBuffer(tr.commandBufferTriangle) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void Shaders::createCommandBufferUI(ThreadResources& tr)
{
}

void Shaders::drawFrame_Triangle(ThreadResources& tr)
{
	if (!enabledTriangle) return;
	//Log("draw index " << engine.currentFrameIndex << endl);

	simpleShader.updatePerFrame(tr);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//VkSemaphore waitSemaphores[] = { tr.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;//waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tr.commandBufferTriangle;
	//VkSemaphore signalSemaphores[] = { tr.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr; // signalSemaphores;
	tr.submitinfos.clear();
	tr.submitinfos.push_back(submitInfo);
}

void Shaders::initiateShader_BackBufferImageDump()
{
	enabledImageDump = true;
	for (auto& res : engine.threadResources) {
		initiateShader_BackBufferImageDumpSingle(res);
	}
}

// SHADER BackBufferImageDump

void Shaders::initiateShader_BackBufferImageDumpSingle(ThreadResources& res)
{
	enabledImageDump = true;
	auto& device = engine.global.device;
	auto& global = engine.global;
	global.createImage(engine.getBackBufferExtent().width, engine.getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, global.ImageFormat, VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		res.imageDumpAttachment.image, res.imageDumpAttachment.memory);
	// Get layout of the image (including row pitch)
	VkImageSubresource subResource{};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkSubresourceLayout subResourceLayout;

	vkGetImageSubresourceLayout(device, res.imageDumpAttachment.image, &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	vkMapMemory(device, res.imageDumpAttachment.memory, 0, VK_WHOLE_SIZE, 0, (void**)&res.imagedata);
	res.imagedata += subResourceLayout.offset;
	res.subResourceLayout = subResourceLayout;
}

void Shaders::executeBufferImageDump(ThreadResources& tr)
{
	if (!enabledImageDump) return;
	auto& device = engine.global.device;
	auto& global = engine.global;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferImageDump) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(tr.commandBufferImageDump, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}

	// Transition destination image to transfer destination layout
	VkImageMemoryBarrier dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcAccessMask = 0;
	dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier.image = tr.imageDumpAttachment.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferImageDump, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.layerCount = 1;
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.layerCount = 1;
	imageCopyRegion.extent.width = engine.getBackBufferExtent().width;
	imageCopyRegion.extent.height = engine.getBackBufferExtent().height;
	imageCopyRegion.extent.depth = 1;

	vkCmdCopyImage(
		tr.commandBufferImageDump,
		tr.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		tr.imageDumpAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageCopyRegion);

	VkImageMemoryBarrier dstBarrier2{};
	dstBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dstBarrier2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dstBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	dstBarrier2.image = tr.imageDumpAttachment.image;
	dstBarrier2.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferImageDump, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier2);
	if (vkEndCommandBuffer(tr.commandBufferImageDump) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
	vkWaitForFences(engine.global.device, 1, &tr.imageDumpFence, VK_TRUE, UINT64_MAX);
	vkResetFences(engine.global.device, 1, &tr.imageDumpFence);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tr.commandBufferImageDump;
	if (vkQueueSubmit(engine.global.graphicsQueue, 1, &submitInfo, tr.imageDumpFence) != VK_SUCCESS) {
		Error("failed to submit draw command buffer!");
	}
	vkDeviceWaitIdle(device);
	vkFreeCommandBuffers(device, tr.commandPool, 1, &tr.commandBufferImageDump);
	// now copy image data to file:
	stringstream name;
	name << "out_" << setw(2) << setfill('0') << imageCouter++ << ".ppm";
	auto filename = engine.files.findFile(name.str(), FileCategory::TEXTURE, false, true);
	if (!engine.files.checkFileForWrite(filename)) {
		return;
	}
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	int32_t height = imageCopyRegion.extent.height;
	int32_t width = imageCopyRegion.extent.width;
	// ppm header
	file << "P6\n" << imageCopyRegion.extent.width << "\n" << imageCopyRegion.extent.height << "\n" << 255 << "\n";

	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	// Check if source is BGR and needs swizzle
	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
	const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());
	const char* imagedata = tr.imagedata;
	// ppm binary pixel data
	for (int32_t y = 0; y < height; y++) {
		unsigned int* row = (unsigned int*)imagedata;
		for (int32_t x = 0; x < width; x++) {
			if (colorSwizzle) {
				file.write((char*)row + 2, 1);
				file.write((char*)row + 1, 1);
				file.write((char*)row, 1);
			}
			else {
				file.write((char*)row, 3);
			}
			row++;
		}
		imagedata += tr.subResourceLayout.rowPitch;
	}
	file.close();
	Log("written image dump file: " << engine.files.absoluteFilePath(filename).c_str() << endl);
}

void Shaders::queueSubmit(ThreadResources& tr)
{
	LogCondF(LOG_QUEUE, "queue submit image index " << tr.frameIndex << endl);
	if (vkQueueSubmit(engine.global.graphicsQueue, 1, &tr.submitinfos.at(0), tr.inFlightFence) != VK_SUCCESS) {
		Error("failed to submit draw command buffer!");
	}
}

Shaders::~Shaders()
{
	Log("Shaders destructor\n");
	if (enabledTriangle) {
		vkDestroyBuffer(engine.global.device, vertexBufferTriangle, nullptr);
		vkFreeMemory(engine.global.device, vertexBufferMemoryTriangle, nullptr);
		vkDestroyBuffer(engine.global.device, indexBufferTriangle, nullptr);
		vkFreeMemory(engine.global.device, indexBufferMemoryTriangle, nullptr);
		vkDestroyShaderModule(engine.global.device, fragShaderModuleTriangle, nullptr);
		vkDestroyShaderModule(engine.global.device, vertShaderModuleTriangle, nullptr);
	}
}