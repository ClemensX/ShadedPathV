#include "mainheader.h"

void ClearShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	shaderState.isClear = true;
	ShaderBase::init(engine);
	int fl = engine.getFramesInFlight();
	for (int i = 0; i < fl; i++) {
		ClearSubShader sub;
		sub.init(this, "ClearSubshader");
		clearSubShaders.push_back(sub);
	}
}

void ClearShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	ClearSubShader& cs = clearSubShaders[tr.threadResourcesIndex];
	cs.initSingle(tr, shaderState);
	cs.allocateCommandBuffer(tr, &cs.commandBuffer, "CLEAR PERMANENT COMMAND BUFFER");
	createRenderPassAndFramebuffer(tr, shaderState, cs.renderPass, cs.framebuffer, cs.framebuffer2);
}

void ClearShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
	shaderState.isClear = false;
}

void ClearShader::createCommandBuffer(ThreadResources& tr)
{
	if (!enabled) return;
	ClearSubShader& sub = clearSubShaders[tr.threadResourcesIndex];
	sub.createGlobalCommandBufferAndRenderPass(tr);
}

void ClearShader::addCurrentCommandBuffer(ThreadResources& tr)
{
	ClearSubShader& cs = clearSubShaders[tr.threadResourcesIndex];
	//tr.activeCommandBuffers.push_back(cs.commandBuffer);
    // TODO: add this secondary buffer to primary command buffer
};

ClearShader::~ClearShader()
{
	if (!enabled) {
		return;
	}
	for (ClearSubShader sub : clearSubShaders) {
		sub.destroy();
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void ClearShader::destroyThreadResources(ThreadResources& tr)
{
}

void ClearSubShader::init(ClearShader* parent, std::string debugName) {
	clearShader = parent;
	name = debugName;
	engine = clearShader->engine;
	device = &engine->globalRendering.device;
	Log("ClearSubShader init: " << debugName.c_str() << std::endl);
}

void ClearSubShader::allocateCommandBuffer(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	engine->util.debugNameObjectCommandBuffer(commandBuffer, debugName);
}

void ClearSubShader::createGlobalCommandBufferAndRenderPass(ThreadResources& tr)
{
	allocateCommandBuffer(tr, &commandBuffer, "CLEAR COMMAND BUFFER");
	addRenderPassAndDrawCommands(tr, &commandBuffer);
}

void ClearSubShader::addRenderPassAndDrawCommands(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr)
{
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
	renderPassInfo.renderArea.extent = this->engine->getBackBufferExtent();

	auto clearColor = clearShader->getClearColor();
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {clearColor.x, clearColor.y, clearColor.z, clearColor.w} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
	if (engine->isStereo()) {
		renderPassInfo.framebuffer = framebuffer2;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(commandBuffer);
	}
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void ClearSubShader::destroy()
{
	vkDestroyFramebuffer(*device, framebuffer, nullptr);
	vkDestroyRenderPass(*device, renderPass, nullptr);
	if (engine->isStereo()) {
		vkDestroyFramebuffer(*device, framebuffer2, nullptr);
	}
}

void EndShader::init(ShadedPathEngine& engine, ShaderState& shaderState)
{
	ClearShader::init(engine, shaderState);
	shaderState.isClear = false;
};
