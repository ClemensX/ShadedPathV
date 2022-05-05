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
    createBackBufferImage();
    createDepthResources();
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

void ThreadResources::createBackBufferImage()
{
    auto& device = engine->global.device;
    auto& global = engine->global;
    // Color attachment
    global.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, global.ImageFormat, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAttachment.image, colorAttachment.memory);
    colorAttachment.view = global.createImageView(colorAttachment.image, global.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void ThreadResources::createCommandPool()
{
    engine->global.createCommandPool(commandPool);
}

void ThreadResources::createDepthResources()
{
    VkFormat depthFormat = engine->global.depthFormat;

    engine->global.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = engine->global.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
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
    vkDestroyFramebuffer(device, framebufferClear, nullptr);
    vkDestroyFramebuffer(device, framebufferSimple, nullptr);
    vkDestroyFramebuffer(device, framebufferUI, nullptr);
    vkDestroyFramebuffer(device, framebufferLine, nullptr);
    vkDestroyFramebuffer(device, framebufferLineAdd, nullptr);
    vkDestroyRenderPass(device, renderPassClear, nullptr);
    vkDestroyRenderPass(device, renderPassSimpleShader, nullptr);
    vkDestroyRenderPass(device, renderPassLine, nullptr);
    vkDestroyRenderPass(device, renderPassLineAdd, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroyImageView(device, imageDumpAttachment.view, nullptr);
    vkDestroyImage(device, imageDumpAttachment.image, nullptr);
    vkFreeMemory(device, imageDumpAttachment.memory, nullptr);
    vkDestroyImageView(device, colorAttachment.view, nullptr);
    vkDestroyImage(device, colorAttachment.image, nullptr);
    vkFreeMemory(device, colorAttachment.memory, nullptr);
    vkDestroyPipeline(device, graphicsPipelineTriangle, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayoutTriangle, nullptr);
    vkDestroyBuffer(device, uniformBufferTriangle, nullptr);
    vkFreeMemory(device, uniformBufferMemoryTriangle, nullptr);
    vkDestroyPipeline(device, graphicsPipelineLine, nullptr);
    vkDestroyPipeline(device, graphicsPipelineLineAdd, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayoutLine, nullptr);
    vkDestroyBuffer(device, uniformBufferLine, nullptr);
    vkFreeMemory(device, uniformBufferMemoryLine, nullptr);
    vkDestroyBuffer(device, vertexBufferAdd, nullptr);
    vkFreeMemory(device, vertexBufferAddMemory, nullptr);
    Log("ThreadResource destructed: " << this << endl);
};

