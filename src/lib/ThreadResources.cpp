#include "mainheader.h"

using namespace std;

// FrameResources

FrameResources::~FrameResources() {
    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    auto& shaders = engine->shaders;
    if (colorAttachment.image) vkDestroyImage(device, colorAttachment.image, nullptr);
    if (depthImage) vkDestroyImage(device, depthImage, nullptr);
    if (imageAvailableSemaphore) vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    if (renderFinishedSemaphore) vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    if (presentFence) vkDestroyFence(device, presentFence, nullptr);
    if (inFlightFence) vkDestroyFence(device, inFlightFence, nullptr);
    if (uiRenderFinished) vkDestroyEvent(device, uiRenderFinished, nullptr);
    if (colorAttachment.memory) vkFreeMemory(device, colorAttachment.memory, nullptr);
    if (depthImageMemory) vkFreeMemory(device, depthImageMemory, nullptr);
    if (colorAttachment.view) vkDestroyImageView(device, colorAttachment.view, nullptr);
    if (depthImageView) vkDestroyImageView(device, depthImageView, nullptr);
    if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
    if (engine && engine->isStereo()) {
        if (colorAttachment2.image) vkDestroyImage(device, colorAttachment2.image, nullptr);
        if (depthImage2) vkDestroyImage(device, depthImage2, nullptr);
        if (colorAttachment2.memory) vkFreeMemory(device, colorAttachment2.memory, nullptr);
        if (depthImageMemory2) vkFreeMemory(device, depthImageMemory2, nullptr);
        if (colorAttachment2.view) vkDestroyImageView(device, colorAttachment2.view, nullptr);
        if (depthImageView2) vkDestroyImageView(device, depthImageView2, nullptr);
    }
    Log("FrameResources destroyed\n");
};

void FrameResources::initAll(ShadedPathEngine* engine)
{
    // FrameResources.index and engine ptr was set in ShadedPathEngine::initGlobal()
    for (FrameResources& res : engine->getFrameResources()) {
        res.init();
    }
}

void FrameResources::init()
{
    createFencesAndSemaphores();
    createBackBufferImage();
    createDepthResources();
    createCommandPool();

}

void FrameResources::createFencesAndSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(engine->globalRendering.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS) {
        Error("failed to create imageAvailableSemaphore for a frame");
    }
    engine->util.debugNameObjectSemaphore(imageAvailableSemaphore, "FrameResources.imageAvailableSemaphore");

    if (vkCreateSemaphore(engine->globalRendering.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        Error("failed to create renderFinishedSemaphore for a frame");
    }
    engine->util.debugNameObjectSemaphore(renderFinishedSemaphore, "FrameResources.renderFinishedSemaphore");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    //if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &imageDumpFence) != VK_SUCCESS) {
    //    Error("failed to create inFlightFence for a frame");
    //}
    //fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &presentFence) != VK_SUCCESS) {
        Error("failed to create presentFence for a frame");
    }
    engine->util.debugNameObjectFence(presentFence, "FrameResources.presentFence");
    //fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    engine->util.debugNameObjectFence(inFlightFence, "FrameResources.inFlightFence");

    VkEventCreateInfo eventInfo{};
    eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    eventInfo.flags = 0;
    if (vkCreateEvent(engine->globalRendering.device, &eventInfo, nullptr, &uiRenderFinished) != VK_SUCCESS) {
        Error("failed to create event uiRenderFinished for a frame");
    }
    engine->util.debugNameObjectEvent(uiRenderFinished, "FrameResources.uiRenderFinished");
}

void FrameResources::createBackBufferImage()
{
    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    // Color attachment
    auto name = engine->util.createDebugName("FrameInfo BackBufferImage", frameIndex);
    global.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, global.ImageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAttachment.image, colorAttachment.memory, name.c_str());
    colorAttachment.view = global.createImageView(colorAttachment.image, global.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    if (engine->isStereo()) {
        auto name = engine->util.createDebugName("FrameInfo Stereo BackBufferImage", frameIndex);
        global.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, global.ImageFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAttachment2.image, colorAttachment2.memory, name.c_str());
        colorAttachment2.view = global.createImageView(colorAttachment2.image, global.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void FrameResources::createDepthResources()
{
    VkFormat depthFormat = engine->globalRendering.depthFormat;

    auto name = engine->util.createDebugName("FrameInfo DepthImage_", frameIndex);
    engine->globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, name.c_str());
    depthImageView = engine->globalRendering.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    if (engine->isStereo()) {
        auto name = engine->util.createDebugName("FrameInfo Stereo DepthImage_", frameIndex);
        engine->globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage2, depthImageMemory2, name.c_str());
        depthImageView2 = engine->globalRendering.createImageView(depthImage2, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
}

void FrameResources::createCommandPool()
{
    drawResults.resize(engine->getParrallelAppDrawCalls()); // one draw result per draw call topic
    for (auto& dr : drawResults) {
        dr.image = nullptr;
        for (auto& cb : dr.commandBuffers) { // initialize command buffers to nullptr
            cb = nullptr;
        }
    }
    engine->globalRendering.createCommandPool(commandPool, engine->util.createDebugName("FrameInfoMainCommandPool_", frameIndex));
}

void FrameResources::clearDrawResults()
{
    for (auto& dr : drawResults) {
        dr.image = nullptr;
        for (auto& cb : dr.commandBuffers) { // initialize command buffers to nullptr
            cb = nullptr;
        }
    }
    numCommandBuffers = 0;
}

// ThreadResources

ThreadResources::~ThreadResources()
{
    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    auto& shaders = engine->shaders;
}

/*
ThreadResources::ThreadResources(ShadedPathEngine* engine_)
{
	Log("ThreadResource c'tor: " << this << endl);
    setEngine(engine_);
    threadResourcesIndex = engine->threadResourcesCount++;
}


void ThreadResources::createFencesAndSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(engine->globalRendering.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS) {
        Error("failed to create imageAvailableSemaphore for a frame");
    }
    if (vkCreateSemaphore(engine->globalRendering.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        Error("failed to create renderFinishedSemaphore for a frame");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &imageDumpFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    //fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &presentFence) != VK_SUCCESS) {
        Error("failed to create presentFence for a frame");
    }
    fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(engine->globalRendering.device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }

    VkEventCreateInfo eventInfo{};
    eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    eventInfo.flags = 0;
    if (vkCreateEvent(engine->globalRendering.device, &eventInfo, nullptr, &uiRenderFinished) != VK_SUCCESS) {
        Error("failed to create event uiRenderFinished for a frame");
    }
}

void ThreadResources::createBackBufferImage()
{
    auto& device = engine->globalRendering.device;
    auto& global = engine->global;
    // Color attachment
    globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, globalRendering.ImageFormat, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAttachment.image, colorAttachment.memory);
    colorAttachment.view = globalRendering.createImageView(colorAttachment.image, globalRendering.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    if (engine->isStereo()) {
        globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, globalRendering.ImageFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAttachment2.image, colorAttachment2.memory);
        colorAttachment2.view = globalRendering.createImageView(colorAttachment2.image, globalRendering.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void ThreadResources::createCommandPool()
{
    engine->globalRendering.createCommandPool(commandPool);
}

void ThreadResources::createDepthResources()
{
    VkFormat depthFormat = engine->globalRendering.depthFormat;

    engine->globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = engine->globalRendering.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    if (engine->isStereo()) {
        engine->globalRendering.createImage(engine->getBackBufferExtent().width, engine->getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage2, depthImageMemory2);
        depthImageView2 = engine->globalRendering.createImageView(depthImage2, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
 }

ThreadResources::~ThreadResources()
{
    auto& device = engine->globalRendering.device;
    auto& global = engine->global;
    auto& shaders = engine->shaders;
    //shaders.clearShader.destroyThreadResources(*this);
    shaders.destroyThreadResources(*this);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, imageDumpFence, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroyFence(device, presentFence, nullptr);
    vkDestroyEvent(device, uiRenderFinished, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroyImageView(device, imageDumpAttachment.view, nullptr);
    vkDestroyImage(device, imageDumpAttachment.image, nullptr);
    vkFreeMemory(device, imageDumpAttachment.memory, nullptr);
    vkDestroyImageView(device, colorAttachment.view, nullptr);
    vkDestroyImage(device, colorAttachment.image, nullptr);
    vkFreeMemory(device, colorAttachment.memory, nullptr);
    if (engine->isStereo()) {
        vkDestroyImageView(device, depthImageView2, nullptr);
        vkDestroyImage(device, depthImage2, nullptr);
        vkFreeMemory(device, depthImageMemory2, nullptr);
        vkDestroyImageView(device, colorAttachment2.view, nullptr);
        vkDestroyImage(device, colorAttachment2.image, nullptr);
        vkFreeMemory(device, colorAttachment2.memory, nullptr);
    }
    Log("ThreadResource destructed: " << this << endl);
};
*/