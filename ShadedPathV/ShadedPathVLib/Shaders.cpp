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

void Shaders::initiateShader_Triangle()
{
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

	// pipelines must be created for every rendering thread
	for (auto &res : engine.threadResources) {
		initiateShader_TriangleSingle(res);
	}
}

void Shaders::initiateShader_TriangleSingle(ThreadResources& res)
{
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
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// viewport and scissors
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)engine.getCurrentExtent().width;
	viewport.height = (float)engine.getCurrentExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = engine.getCurrentExtent();

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
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
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
	pipelineInfo.renderPass = res.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(engine.global.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &res.graphicsPipelineTriangle) != VK_SUCCESS) {
		Error("failed to create graphics pipeline!");
	}
}

void Shaders::initiateShader_BackBufferImageDump()
{
	for (auto& res : engine.threadResources) {
		initiateShader_BackBufferImageDumpSingle(res);
	}
}

void Shaders::initiateShader_BackBufferImageDumpSingle(ThreadResources& res)
{
	auto& device = engine.global.device;
	auto& global = engine.global;
	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = engine.global.ImageFormat;
	image.extent.width = engine.getCurrentExtent().width;
	image.extent.height = engine.getCurrentExtent().height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;

	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_LINEAR;
	image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	if (vkCreateImage(device, &image, nullptr, &res.imageDumpAttachment.image) != VK_SUCCESS) {
		Error("failed to create image dump image!");
	}
	vkGetImageMemoryRequirements(device, res.imageDumpAttachment.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = global.findMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vkAllocateMemory(device, &memAlloc, nullptr, &res.imageDumpAttachment.memory) != VK_SUCCESS) {
		Error("failed to allocate image dump memory");
	}
	if (vkBindImageMemory(device, res.imageDumpAttachment.image, res.imageDumpAttachment.memory, 0) != VK_SUCCESS) {
		Error("failed to bind image memory");
	}

}

bool Shaders::shouldClose()
{
	return false;
}

void Shaders::recordDrawCommand_Triangle(VkCommandBuffer& commandBuffer, ThreadResources& tr)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr.graphicsPipelineTriangle);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Shaders::drawFrame_Triangle()
{
	// select the right thread resources
	auto& tr = engine.threadResources[engine.currentFrameIndex];
	//Log("draw index " << engine.currentFrameIndex << endl);

	// wait for fence signal
	vkWaitForFences(engine.global.device, 1, &tr.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(engine.global.device, 1, &tr.inFlightFence);


	//uint32_t imageIndex;
	//vkAcquireNextImageKHR(engine.device, engine.swapChain, UINT64_MAX, tr.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
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
	if (vkQueueSubmit(engine.global.graphicsQueue, 1, &submitInfo, tr.inFlightFence) != VK_SUCCESS) {
		Error("failed to submit draw command buffer!");
	}
	//VkPresentInfoKHR presentInfo{};
	//presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//presentInfo.waitSemaphoreCount = 1;
	//presentInfo.pWaitSemaphores = signalSemaphores;

	//VkSwapchainKHR swapChains[] = { engine.swapChain };
	//presentInfo.swapchainCount = 1;
	//presentInfo.pSwapchains = swapChains;
	//presentInfo.pImageIndices = &imageIndex;
	//presentInfo.pResults = nullptr; // Optional
	//vkQueuePresentKHR(engine.presentQueue, &presentInfo);

	//const uint64_t waitValue = 1;
	//VkSemaphore signalWaitSemaphores[] = { tr.renderFinishedSemaphore };
	//VkSemaphoreWaitInfo waitInfo;
	//waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	//waitInfo.pNext = NULL;
	//waitInfo.flags = 0;
	//waitInfo.semaphoreCount = 0;
	//waitInfo.pSemaphores = nullptr;//signalWaitSemaphores;
	//waitInfo.pValues = &waitValue;
	//vkWaitSemaphores(engine.global.device, &waitInfo, UINT64_MAX);
	ThemedTimer::getInstance()->add("DrawFrame");
}

Shaders::~Shaders()
{
	Log("Shaders destructor\n");
	vkDestroyShaderModule(engine.global.device, fragShaderModuleTriangle, nullptr);
	vkDestroyShaderModule(engine.global.device, vertShaderModuleTriangle, nullptr);
}