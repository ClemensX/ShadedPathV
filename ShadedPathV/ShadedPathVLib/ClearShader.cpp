#include "pch.h"

void ClearShader::createDescriptorSetLayout()
{
}

void ClearShader::createDescriptorSets(ThreadResources& res)
{
}

void ClearShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	shaderState.isClear = true;
	ShaderBase::init(engine);
}

void ClearShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
	createRenderPassAndFramebuffer(tr, shaderState, tr.renderPassClear, tr.framebufferClear);
}

void ClearShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
	shaderState.isClear = false;
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
	renderPassInfo.framebuffer = tr.framebufferClear;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = this->engine->getBackBufferExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	if (!GlobalRendering::USE_PROFILE_DYN_RENDERING) {
			vkCmdBeginRenderPass(tr.commandBufferClear, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdEndRenderPass(tr.commandBufferClear);
	} else {
		VkRenderingAttachmentInfoKHR color_attachment_info{};
		color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		color_attachment_info.imageView = tr.colorAttachment.view;
		color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
		color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_info.clearValue = clearValues[0];

		VkRenderingAttachmentInfoKHR depth_attachment_info{};
		depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depth_attachment_info.imageView = tr.depthImageView;
		depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
		depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment_info.clearValue = clearValues[1];

		// Transition color attachment image to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		VkImageMemoryBarrier dstBarrier{};
		dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		dstBarrier.srcAccessMask = 0;
		dstBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		dstBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		dstBarrier.image = tr.colorAttachment.image;
		dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(tr.commandBufferClear, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

		// Transition depth attachment image to LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		dstBarrier.srcAccessMask = 0;
		dstBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		dstBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		dstBarrier.image = tr.depthImage;
		dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(tr.commandBufferClear, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

		//auto render_area = VkRect2D{ VkOffset2D{}, VkExtent2D{width, height} };
		//auto render_info = vkb::initializers::rendering_info(render_area, 1, &color_attachment_info);
		VkRenderingInfo render_info{};
		render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		render_info.colorAttachmentCount = 1;
		render_info.flags = 0;
		render_info.layerCount = 1;
		render_info.pColorAttachments = &color_attachment_info;
		render_info.pDepthAttachment = &depth_attachment_info;
		render_info.pStencilAttachment = &depth_attachment_info;
		render_info.renderArea.offset = { 0, 0 };
		render_info.renderArea.extent = this->engine->getBackBufferExtent();

		vkCmdBeginRendering(tr.commandBufferClear, &render_info);

		vkCmdEndRendering(tr.commandBufferClear);
	}
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
