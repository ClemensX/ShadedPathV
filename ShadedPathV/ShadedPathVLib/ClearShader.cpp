#include "pch.h"

void ClearShader::createDescriptorSetLayout()
{
}

void ClearShader::createDescriptorSets(ThreadResources& res)
{
}

void ClearShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
}

void ClearShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	createRenderPassAndFramebuffer(tr, shaderState, tr.renderPassClear, tr.framebuffer);
}

void ClearShader::createCommandBuffer(ThreadResources& tr)
{
	if (!enabled) return;
	auto& device = this->engine->global.device;
	auto& global = this->engine->global;
	auto& shaders = this->engine->shaders;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferClear) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(tr.commandBufferClear, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = tr.renderPassClear;
	renderPassInfo.framebuffer = tr.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->engine->getBackBufferExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(tr.commandBufferClear, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(tr.commandBufferClear);
	if (vkEndCommandBuffer(tr.commandBufferClear) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
}

void ClearShader::addCurrentCommandBuffer(ThreadResources& tr) {
	tr.activeCommandBuffers.push_back(tr.commandBufferClear);
};

ClearShader::~ClearShader()
{
	if (!enabled) {
		return;
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}
