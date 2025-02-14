#include "mainheader.h"

using namespace std;

void UIShader::init(ShadedPathEngine &engine, ShaderState& shaderState)
{
    if (engine.presentation.windowInfo == nullptr) {
        return;
    }
	ShaderBase::init(engine);
	engine.ui.init(&engine);
    int fl = engine.getFramesInFlight();
    for (int i = 0; i < fl; i++) {
        // per frame
        UISubShader pf;
        pf.init(this, "PerFrameUISubshader");
        perFrameSubShaders.push_back(pf);
    }
    for (auto& fi : engine.getFrameResources()) {
        initSingle(fi, shaderState);
    }

}

void UIShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
    if (!enabled) {
        return;
    }
    UISubShader& pf = perFrameSubShaders[tr.frameIndex];
    pf.initSingle(tr, shaderState);
    auto name = engine->util.createDebugName("UI COMMAND BUFFER", tr.frameIndex);
    pf.allocateCommandBuffer(tr, name.c_str());
}

void UIShader::finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState)
{
}

void UIShader::createCommandBuffer(FrameResources& tr)
{
}

void UIShader::addCurrentCommandBuffer(FrameResources& tr) {
};

void UIShader::addCommandBuffers(FrameResources* fr, DrawResult* drawResult)
{
}

void UIShader::destroyThreadResources(FrameResources& tr)
{
}

UIShader::~UIShader()
{
	if (!enabled) {
		return;
	}
	//vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    for (UISubShader& sub : perFrameSubShaders) {
        sub.destroy();
    }
}

void UIShader::draw(FrameResources* fr, WindowInfo* winfo, GPUImage* srcImage)
{
    if (enabled)
    {
        if (srcImage->isRightEye) {
            return;
        }
        auto& tr = *fr;
        UISubShader& pf = perFrameSubShaders[tr.frameIndex];
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(pf.commandBuffer, &beginInfo) != VK_SUCCESS) {
            Error("failed to begin recording back buffer copy command buffer!");
        }
        // Transition src image to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        DirectImage::toLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            pf.commandBuffer, srcImage);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = engine->ui.imGuiRenderPass;//tr.renderPassDraw;
        renderPassInfo.framebuffer = pf.framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = engine->getBackBufferExtent();
        renderPassInfo.clearValueCount = 0;
        vkCmdBeginRenderPass(pf.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        //recordDrawCommand_Triangle(tr.commandBufferTriangle, tr);
        engine->ui.render(fr, &pf);
        vkCmdEndRenderPass(pf.commandBuffer);
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
        if (vkEndCommandBuffer(pf.commandBuffer) != VK_SUCCESS) {
            Error("failed to record back buffer copy command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { winfo->imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &pf.commandBuffer;
        VkSemaphore signalSemaphores[] = { tr.imageAvailableSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        //vkDeviceWaitIdle(global.device); does not help
        //engine->log_current_thread();
        LogCondF(LOG_FENCE, "queue thread submit present fence " << hex << ThreadInfo::thread_osid() << endl);
        if (vkQueueSubmit(engine->globalRendering.graphicsQueue, 1, &submitInfo, nullptr/*tr.presentFence*/) != VK_SUCCESS) {
            Error("failed to submit draw command buffer!");
        }
        // force image format transition:
        srcImage->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void UISubShader::init(UIShader * parent, std::string debugName) {
    uiShader = parent;
    name = debugName;
    engine = uiShader->engine;
    device = engine->globalRendering.device;
    Log("UIubShader init: " << debugName.c_str() << std::endl);
}
    
void UISubShader::initSingle(FrameResources& tr, ShaderState& shaderState)
{
    VkFramebufferCreateInfo framebufferInfo{};
    VkImageView attachmentsUI[] = {
        tr.colorImage.fba.view
    };
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = engine->ui.imGuiRenderPass;// renderPassDraw;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachmentsUI;
    framebufferInfo.width = engine->getBackBufferExtent().width;
    framebufferInfo.height = engine->getBackBufferExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->globalRendering.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        Error("failed to create framebuffer!");
    }
}
    
void UISubShader::allocateCommandBuffer(FrameResources& tr, const char* debugName)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tr.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        Error("failed to allocate command buffers!");
    }
    engine->util.debugNameObjectCommandBuffer(commandBuffer, debugName);
}

void UISubShader::destroy() {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
}
