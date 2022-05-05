#include "pch.h"

void UIShader::createDescriptorSetLayout()
{
}

void UIShader::createDescriptorSets(ThreadResources& res)
{
}

void UIShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
	ShaderBase::init(engine);
	engine.ui.enable();
	engine.ui.init(&engine);

}

void UIShader::initSingle(ThreadResources& tr, ShaderState& shaderState)
{
    VkFramebufferCreateInfo framebufferInfo{};
    VkImageView attachmentsUI[] = {
        tr.colorAttachment.view
    };
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = engine->ui.imGuiRenderPass;// renderPassDraw;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachmentsUI;
    framebufferInfo.width = engine->getBackBufferExtent().width;
    framebufferInfo.height = engine->getBackBufferExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->global.device, &framebufferInfo, nullptr, &tr.framebufferUI) != VK_SUCCESS) {
        Error("failed to create framebuffer!");
    }
}

void UIShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void UIShader::createCommandBuffer(ThreadResources& tr)
{
}

void UIShader::addCurrentCommandBuffer(ThreadResources& tr) {
};

UIShader::~UIShader()
{
	if (!enabled) {
		return;
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void UIShader::draw(ThreadResources& tr)
{
    if (enabled)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(tr.commandBufferUI, &beginInfo) != VK_SUCCESS) {
            Error("failed to begin recording back buffer copy command buffer!");
        }
        // Transition src image to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier dstBarrier{};
        dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        dstBarrier.srcAccessMask = 0;
        dstBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        dstBarrier.image = tr.colorAttachment.image;
        dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(tr.commandBufferUI, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = engine->ui.imGuiRenderPass;//tr.renderPassDraw;
        renderPassInfo.framebuffer = tr.framebufferUI;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = engine->getBackBufferExtent();
        renderPassInfo.clearValueCount = 0;
        vkCmdBeginRenderPass(tr.commandBufferUI, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        //recordDrawCommand_Triangle(tr.commandBufferTriangle, tr);
        engine->ui.render(tr);
        vkCmdEndRenderPass(tr.commandBufferUI);
        // Transition src image back to to LAYOUT_TRANSFER_SRC_OPTIMAL
        //dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        //dstBarrier.srcAccessMask = 0;
        //dstBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        //dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        //dstBarrier.image = tr.colorAttachment.image;
        //dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        //vkCmdPipelineBarrier(tr.commandBufferPresentBack, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        //    0, 0, nullptr, 0, nullptr, 1, &dstBarrier);
        //vkCmdSetEvent(tr.commandBufferPresentBack, tr.uiRenderFinished, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        //vkCmdWaitEvents(tr.commandBufferPresentBack, 1, &tr.uiRenderFinished, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        //    0, nullptr, 0, nullptr, 0, nullptr);
        //vkCmdResetEvent(tr.commandBufferPresentBack, tr.uiRenderFinished, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        if (vkEndCommandBuffer(tr.commandBufferUI) != VK_SUCCESS) {
            Error("failed to record back buffer copy command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { tr.imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &tr.commandBufferUI;
        VkSemaphore signalSemaphores[] = { tr.imageAvailableSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        //vkDeviceWaitIdle(global.device); does not help
        LogCondF(LOG_FENCE, "queue thread submit present fence " << hex << ThreadInfo::thread_osid() << endl);
        if (vkQueueSubmit(engine->global.graphicsQueue, 1, &submitInfo, nullptr/*tr.presentFence*/) != VK_SUCCESS) {
            Error("failed to submit draw command buffer!");
        }

    }
}