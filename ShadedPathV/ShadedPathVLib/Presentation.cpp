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
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

void Presentation::presentBackBufferImage()
{
    // select the right thread resources
    auto& tr = engine.threadResources[engine.currentFrameIndex];
    auto& device = engine.global.device;
    auto& global = engine.global;

    // wait for fence signal
    vkWaitForFences(device, 1, &tr.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &tr.inFlightFence);


    uint32_t imageIndex;
    if (vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, tr.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS) {
        Error("cannot aquire next image KHR");
    }
}

Presentation::~Presentation() {
    // destroy swap chain image views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(engine.global.device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(engine.global.device, swapChain, nullptr);
    vkDestroySurfaceKHR(engine.global.vkInstance, surface, nullptr);
    Log("Presentation destructor\n");
};
