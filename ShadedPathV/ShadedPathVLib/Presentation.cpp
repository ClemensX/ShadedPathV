#include "pch.h"

void Presentation::init()
{
    if (!enabled) return;
    createSurface();
}

void Presentation::initAfterDeviceCreation()
{
    if (!enabled) return;
    createSwapChain();
    createImageViews();
}

void Presentation::initGLFW()
{
    if (!enabled) return;
    glfwInit();
    if (true) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(engine.win_width, engine.win_height, engine.win_name, nullptr, nullptr);
        // validate requested window size:
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width != engine.win_width || height != engine.win_height) {
            Error("Could not create window with requested size");
        }
    }
}

void Presentation::pollEvents()
{
    glfwPollEvents();
}

bool Presentation::shouldClose()
{
	return glfwWindowShouldClose(window);
}

void Presentation::possiblyAddDeviceExtensions(vector<const char*> &ext)
{
	if (enabled) {
		for (auto& s : deviceExtensions)
			ext.push_back(s);
	}
}

void Presentation::createSurface()
{
    if (glfwCreateWindowSurface(engine.global.vkInstance, window, nullptr, &surface) != VK_SUCCESS) {
        Error("failed to create window surface!");
    }

}

SwapChainSupportDetails Presentation::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR Presentation::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, bool listmode) {
    for (const auto& availableFormat : availableFormats) {
        if (listmode) {
            Log("swap chain possible format: " << availableFormat.format << " color Space: " << availableFormat.colorSpace << endl);
        }
        else {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
    }
    if (listmode) return availableFormats[0];
    Error("could not find right swap chain format");
    return availableFormats[0];
}

// list available modes or sleect preferred one
VkPresentModeKHR Presentation::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (listmode) {
            Log("swap chain possible present mode: " << availablePresentMode << endl);
        }
        else {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // enable tripple buffering: newer images replace older ones not yet displayed from the queue
                return availablePresentMode;
            }
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Presentation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(nullptr /* TODO window */, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Presentation::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine.global.physicalDevice);

    // list available modes
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, true);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, true);
    // select preferred mode
    surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    Log("swap chain min max images: " << swapChainSupport.capabilities.minImageCount << " " << swapChainSupport.capabilities.maxImageCount << endl);
    imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT| VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = engine.global.findQueueFamilies(engine.global.physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    // we only want EXCLUSIVE mode:
    if (createInfo.imageSharingMode != VK_SHARING_MODE_EXCLUSIVE) {
        Error("VK_SHARING_MODE_EXCLUSIVE required");
    }
    // no transform necessary
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    // no alpha blending with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    // only one fixed swap chain - no resizing
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(engine.global.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        Error("failed to create swap chain!");
    }
    // retrieve swap chain images:
    vkGetSwapchainImagesKHR(engine.global.device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine.global.device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    Log("swap chain create with # images: " << imageCount << endl);
}

void Presentation::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(engine.global.device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            Error("failed to create image views!");
        }
    }
}

void Presentation::createPresentQueue(unsigned int value)
{
    if (!enabled) return;
    vkGetDeviceQueue(engine.global.device, value, 0, &presentQueue);
}

void Presentation::initBackBufferPresentationSingle(ThreadResources &res)
{
    auto& device = engine.global.device;
    auto& global = engine.global;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = res.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &res.commandBufferPresentBack) != VK_SUCCESS) {
        Error("failed to allocate command buffers!");
    }
}

void Presentation::initBackBufferPresentation()
{
    if (!enabled) return;
    for (auto& res : engine.threadResources) {
        initBackBufferPresentationSingle(res);
    }
}

void Presentation::presentBackBufferImage(ThreadResources& tr)
{
    if (!enabled) return;
    // select the right thread resources
    auto& device = engine.global.device;
    auto& global = engine.global;

    // wait for fence signal
    LogCondF(LOG_QUEUE, "wait present fence image index " << tr.frameIndex << endl);
    vkWaitForFences(device, 1, &tr.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &tr.inFlightFence);
    LogCondF(LOG_QUEUE, "signalled present fence image index " << tr.frameIndex << endl);

    uint32_t imageIndex;
    if (vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, tr.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS) {
        Error("cannot aquire next image KHR");
    }

    //VkCommandBufferAllocateInfo allocInfo{};
    //allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //allocInfo.commandPool = res.commandPool;
    //allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //allocInfo.commandBufferCount = (uint32_t)1;

    //if (vkAllocateCommandBuffers(device, &allocInfo, &res.commandBufferPresentBack) != VK_SUCCESS) {
    //    Error("failed to allocate command buffers!");
    //}
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(tr.commandBufferPresentBack, &beginInfo) != VK_SUCCESS) {
        Error("failed to begin recording back buffer copy command buffer!");
    }

    // UI code
    if (true)
    {
        // Transition src image to LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier dstBarrier{};
        dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        dstBarrier.srcAccessMask = 0;
        dstBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        dstBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        dstBarrier.image = tr.colorAttachment.image;
        dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(tr.commandBufferPresentBack, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = tr.renderPassDraw;
        renderPassInfo.framebuffer = tr.framebufferDraw;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = engine.getBackBufferExtent();
        VkClearValue clearColor = { {{0.0f, 0.0f, 1.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 0;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(tr.commandBufferPresentBack, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        //recordDrawCommand_Triangle(tr.commandBufferTriangle, tr);
        engine.ui.render(tr);
        vkCmdEndRenderPass(tr.commandBufferPresentBack);
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

    }

    // Transition destination image to transfer destination layout
    VkImageMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = 0;
    dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstBarrier.image = this->swapChainImages[imageIndex];
    dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(tr.commandBufferPresentBack, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

    //VkImageCopy imageCopyRegion{};
    //imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //imageCopyRegion.srcSubresource.layerCount = 1;
    //imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //imageCopyRegion.dstSubresource.layerCount = 1;
    //imageCopyRegion.extent.width = engine.getBackBufferExtent().width;
    //imageCopyRegion.extent.height = engine.getBackBufferExtent().height;
    //imageCopyRegion.extent.depth = 1;

    //vkCmdCopyImage(
    //    res.commandBufferPresentBack,
    //    res.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //    this->swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    1,
    //    &imageCopyRegion);

    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSizeSrc;
    blitSizeSrc.x = engine.getBackBufferExtent().width;
    blitSizeSrc.y = engine.getBackBufferExtent().height;
    blitSizeSrc.z = 1;
    VkOffset3D blitSizeDst;
    blitSizeDst.x = engine.win_width;
    blitSizeDst.y = engine.win_height;
    blitSizeDst.z = 1;

    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSizeSrc;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSizeDst;

    vkCmdBlitImage(
        tr.commandBufferPresentBack,
        tr.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        this->swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageBlitRegion,
        VK_FILTER_LINEAR
        );

    VkImageMemoryBarrier dstBarrier2{};
    dstBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dstBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstBarrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    dstBarrier2.image = this->swapChainImages[imageIndex];
    dstBarrier2.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(tr.commandBufferPresentBack, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &dstBarrier2);
    if (vkEndCommandBuffer(tr.commandBufferPresentBack) != VK_SUCCESS) {
        Error("failed to record back buffer copy command buffer!");
    }
    //vkWaitForFences(engine.global.device, 1, &res.imageDumpFence, VK_TRUE, UINT64_MAX);
    //vkResetFences(engine.global.device, 1, &res.imageDumpFence);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { tr.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tr.commandBufferPresentBack;
    VkSemaphore signalSemaphores[] = { tr.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //vkDeviceWaitIdle(global.device); does not help
    LogCondF(LOG_FENCE, "queue thread submit present fence " << hex << ThreadInfo::thread_osid() << endl);
    if (vkQueueSubmit(engine.global.graphicsQueue, 1, &submitInfo, tr.presentFence) != VK_SUCCESS) {
        Error("failed to submit draw command buffer!");
    }
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(presentQueue, &presentInfo);
    //LogF("Frame presented: " << tr.frameNum << endl);
}

Presentation::~Presentation() {
    if (!enabled) return;
    // destroy swap chain image views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(engine.global.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(engine.global.device, swapChain, nullptr);
    vkDestroySurfaceKHR(engine.global.vkInstance, surface, nullptr);
    Log("Presentation destructor\n");
};
