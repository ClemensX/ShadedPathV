#include "pch.h"

ShadedPathEngine* ThreadResources::engine = nullptr;

ThreadResources::ThreadResources()
{
	Log("ThreadResource c'tor: " << this << endl);
}

void ThreadResources::initAll(ShadedPathEngine* engine)
{
	ThreadResources::engine = engine;
	for (ThreadResources &res : engine->threadResources) {
		res.init();
	}
}

void ThreadResources::init()
{
    createFencesAndSemaphores();
    createRenderPassInit();
    createRenderPassDraw();
    createBackBufferImage();
    createFrameBuffer();
    createCommandPool();
}

void ThreadResources::createFencesAndSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(engine->global.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS) {
        Error("failed to create imageAvailableSemaphore for a frame");
    }
    if (vkCreateSemaphore(engine->global.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        Error("failed to create renderFinishedSemaphore for a frame");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    if (vkCreateFence(engine->global.device, &fenceInfo, nullptr, &imageDumpFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    //fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->global.device, &fenceInfo, nullptr, &presentFence) != VK_SUCCESS) {
        Error("failed to create presentFence for a frame");
    }
    fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->global.device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }

    VkEventCreateInfo eventInfo{};
    eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    eventInfo.flags = 0;
    if (vkCreateEvent(engine->global.device, &eventInfo, nullptr, &uiRenderFinished) != VK_SUCCESS) {
        Error("failed to create event uiRenderFinished for a frame");
    }
}

void ThreadResources::createRenderPassInit()
{
    // attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = engine->global.ImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // subpasses and attachment references
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // subpasses
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(engine->global.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        Error("failed to create render pass!");
    }
}

void ThreadResources::createRenderPassDraw()
{
    // attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = engine->global.ImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // subpasses and attachment references
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // subpasses
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(engine->global.device, &renderPassInfo, nullptr, &renderPassDraw) != VK_SUCCESS) {
        Error("failed to create render pass!");
    }
}

void ThreadResources::createBackBufferImage()
{
    auto& device = engine->global.device;
    auto& global = engine->global;
    // Color attachment
    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = global.ImageFormat;
    image.extent.width = engine->getBackBufferExtent().width;
    image.extent.height = engine->getBackBufferExtent().height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    if (vkCreateImage(device, &image, nullptr, &colorAttachment.image) != VK_SUCCESS) {
        Error("failed to create render image!");
    }
    vkGetImageMemoryRequirements(device, colorAttachment.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = global.findMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &memAlloc, nullptr, &colorAttachment.memory) != VK_SUCCESS) {
        Error("failed to allocate image memory");
    }
    if (vkBindImageMemory(device, colorAttachment.image, colorAttachment.memory, 0) != VK_SUCCESS) {
        Error("failed to bind image memory");
    }

    VkImageViewCreateInfo colorImageView{};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = global.ImageFormat;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = colorAttachment.image;
    if (vkCreateImageView(device, &colorImageView, nullptr, &colorAttachment.view) != VK_SUCCESS) {
        Error("failed to create image view");
    }
}

void ThreadResources::createFrameBuffer()
{
    VkImageView attachments[] = {
        colorAttachment.view
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = engine->getBackBufferExtent().width;
    framebufferInfo.height = engine->getBackBufferExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->global.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        Error("failed to create framebuffer!");
    }
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPassDraw;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = engine->getBackBufferExtent().width;
    framebufferInfo.height = engine->getBackBufferExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(engine->global.device, &framebufferInfo, nullptr, &framebufferDraw) != VK_SUCCESS) {
        Error("failed to create framebuffer!");
    }
}

void ThreadResources::createCommandPool()
{
    engine->global.createCommandPool(commandPool);
}

ThreadResources::~ThreadResources()
{
    auto& device = engine->global.device;
    auto& global = engine->global;
    auto& shaders = engine->shaders;
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, imageDumpFence, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyFence(device, presentFence, nullptr);
    vkDestroyEvent(device, uiRenderFinished, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyFramebuffer(device, framebufferDraw, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyRenderPass(device, renderPassDraw, nullptr);
    vkDestroyImageView(device, imageDumpAttachment.view, nullptr);
    vkDestroyImage(device, imageDumpAttachment.image, nullptr);
    vkFreeMemory(device, imageDumpAttachment.memory, nullptr);
    vkDestroyImageView(device, colorAttachment.view, nullptr);
    vkDestroyImage(device, colorAttachment.image, nullptr);
    vkFreeMemory(device, colorAttachment.memory, nullptr);
    vkDestroyPipeline(device, graphicsPipelineTriangle, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayoutTriangle, nullptr);
    // destroy swap chain image views
    //for (auto imageView : swapChainImageViews) {
    //    vkDestroyImageView(device, imageView, nullptr);
    //}
    Log("ThreadResource destructed: " << this << endl);
};

