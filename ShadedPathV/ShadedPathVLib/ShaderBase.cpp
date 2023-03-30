#include "pch.h"

using namespace std;

void ShaderBase::init(ShadedPathEngine& engine)
{
	if (enabled) {
		Error("Shader already initialized!");
	}
	engine.shaders.checkShaderState(engine);
	this->device = engine.global.device;
	this->global = &engine.global;
	this->engine = &engine;
	resources.init(&engine);
	enabled = true;
}

void ShaderBase::createUniformBuffer(ThreadResources& res, VkBuffer& uniformBuffer, size_t size, VkDeviceMemory& uniformBufferMemory)
{
	VkDeviceSize bufferSize = size;
	// we do not want to handle zero length buffers
	if (bufferSize == 0) {
		stringstream s;
		s << "Cannot create Uniform buffer with size " << bufferSize << ". (Did you forget to subclass buffer definition ? )" << endl;
		Error(s.str());
	}
	global->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer, uniformBufferMemory);
}

void ShaderBase::createVertexBuffer(ThreadResources& res, VkBuffer& uniformBuffer, size_t size, VkDeviceMemory& uniformBufferMemory)
{
	VkDeviceSize bufferSize = size;
	// we do not want to handle zero length buffers
	if (bufferSize == 0) {
		stringstream s;
		s << "Cannot create Uniform buffer with size " << bufferSize << ". (Did you forget to subclass buffer definition ? )" << endl;
		Error(s.str());
	}
	global->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer, uniformBufferMemory);
}

void ShaderBase::createDescriptorPool(vector<VkDescriptorPoolSize> &poolSizes, uint32_t overrideMaxSet)
{
	vector<VkDescriptorPoolSize> independent;
	createDescriptorPool(poolSizes, independent, overrideMaxSet);
}

void ShaderBase::createDescriptorPool(vector<VkDescriptorPoolSize>& poolSizes, vector<VkDescriptorPoolSize>& threadIndependentPoolSizes, uint32_t overrideMaxSet)
{
	// duplicate the poolSize for every render thread we have:
	vector<VkDescriptorPoolSize> allThreadsPoolSizes;
	for (int i = 0; i < engine->getFramesInFlight(); i++) {
		copy(poolSizes.begin(), poolSizes.end(), back_inserter(allThreadsPoolSizes));
	}
	copy(threadIndependentPoolSizes.begin(), threadIndependentPoolSizes.end(), back_inserter(allThreadsPoolSizes));
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(allThreadsPoolSizes.size());
	poolInfo.pPoolSizes = allThreadsPoolSizes.data();
	if (overrideMaxSet > 0) {
		poolInfo.maxSets = overrideMaxSet;
	}
	else {
		poolInfo.maxSets = 5; // arbitrary number for now TODO: see if this can be calculated
	}
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		Error("failed to create descriptor pool!");
	}
}

void ShaderBase::createRenderPassAndFramebuffer(ThreadResources& tr, ShaderState shaderState, VkRenderPass& renderPass, VkFramebuffer& frameBuffer, VkFramebuffer& frameBuffer2)
{
	// depth buffer attachement
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = engine->global.depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	if (shaderState.isClear) {
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// attachment
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = engine->global.ImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // last shader
	if (shaderState.isClear) {
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else if (isLastShader()) {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // last shader
	}

	// subpasses and attachment references
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// subpasses
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// render pass
	array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(engine->global.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		Error("failed to create render pass!");
	}

	// frame buffer
	array<VkImageView, 2> attachmentsView = { tr.colorAttachment.view, tr.depthImageView };

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachmentsView.data();
	framebufferInfo.width = static_cast<uint32_t>(shaderState.viewport.width);
	framebufferInfo.height = static_cast<uint32_t>(shaderState.viewport.height);
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(engine->global.device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
		Error("failed to create framebuffer!");
	}
	if (engine->isStereo()) {
		array<VkImageView, 2> attachmentsView2 = { tr.colorAttachment2.view, tr.depthImageView2 };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachmentsView2.data();
		framebufferInfo.width = static_cast<uint32_t>(shaderState.viewport.width);
		framebufferInfo.height = static_cast<uint32_t>(shaderState.viewport.height);
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(engine->global.device, &framebufferInfo, nullptr, &frameBuffer2) != VK_SUCCESS) {
			Error("failed to create framebuffer!");
		}
	}
}

void ShaderBase::startUpdateThread()
{
	if (shaderUpdateQueueInfo.threadRunning) {
		Error("shader update thread can only be started once!");
		return;
	}
	void* native_handle = engine->getThreadGroup().add_t(runUpdateThread, this);
	wstring mod_name = wstring(L"shader_update");//.append(L"_").append(to_wstring(i));
	SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
}

void ShaderBase::runUpdateThread(ShaderBase* shader_instance)
{
	LogCondF(LOG_QUEUE, "run shader update thread" << endl);
	shader_instance->shaderUpdateQueueInfo.threadRunning = true;
	//this_thread::sleep_for(chrono::milliseconds(1000 * (10 - tr->frameIndex)));
	while (shader_instance->engine->isShutdown() == false) {
		// wait until queue submit thread issued all present commands
//        tr->renderThreadContinue->wait(false);
		optional<int> i = shader_instance->shaderUpdateQueue.pop();
		if (!i) {
			break;
		}
		// update using slot i
		shader_instance->update(i.value());
		LogCondF(LOG_QUEUE, "updated shader data " << endl);
	}
	LogCondF(LOG_QUEUE, "run shader update thread end" << endl);
}

ShaderBase::~ShaderBase()
{
}
