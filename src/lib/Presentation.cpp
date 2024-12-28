#include "mainheader.h"

using namespace std;

void glfwErrorCallback(int error, const char* description) {
    Log("GLFW Error (" << error << "): " << description << endl);
    //cerr << "GLFW Error (" << error << "): " << description << endl;
}

Presentation::Presentation(ShadedPathEngine* s) {
    Log("Presentation c'tor\n");
    setEngine(s);
    if (!glfwInitCalled) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            Error("GLFW init failed\n");
        }
        glfwInitCalled = true;
    }
};

Presentation::~Presentation()
{
	Log("Presentation destructor\n");
    if (glfwInitCalled) {
        glfwTerminate();
    }
}

void Presentation::createWindow(WindowInfo* winfo, int w, int h, const char* name,
    bool handleKeyEvents, bool handleMouseMoveEevents, bool handleMouseButtonEvents)
{
    if (!engine->isMainThread()) Error("window creation has to be done from main thread");
    // Get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    // Retrieve the desktop size
    int desktopWidth = videoMode->width;
    int desktopHeight = videoMode->height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(w, h, name, nullptr, nullptr);
    winfo->glfw_window = window;
    winfo->title = name;
    windowInfo = winfo;
    if (engine->isDebugWindowPosition()) {
        // Set window position to right half near the top of the screen
        glfwSetWindowPos(window, desktopWidth / 2, 30);
    }
    // validate requested window size:
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width != w || height != h) {
        Error("Could not create window with requested size");
    }
    if (glfwCreateWindowSurface(engine->globalRendering.vkInstance, window, nullptr, &winfo->surface) != VK_SUCCESS) {
        Error("failed to create window surface!");
    }

    // init callbacks: we assume that no other callback was installed (yet)
    if (handleKeyEvents) {
        // we need a static member function that can be registered with glfw:
        // static auto callback = bind(&Presentation::key_callbackMember, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
        // the above works, but can be done more elegantly with a lambda expression:
        static auto callback_static = [this](GLFWwindow* window, int key, int scancode, int action, int mods) {
            // because we have a this pointer we are now able to call a non-static member method:
            callbackKey(window, key, scancode, action, mods);
            };
        auto old = glfwSetKeyCallback(window,
            [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                // only static methods can be called here as we cannot change glfw function parameter list to include instance pointer
                callback_static(window, key, scancode, action, mods);
            }
        );
        assert(old == nullptr);
    }
    if (handleMouseMoveEevents) {
        static auto callback_static = [this](GLFWwindow* window, double xpos, double ypos) {
            callbackCursorPos(window, xpos, ypos);
            };
        auto old = glfwSetCursorPosCallback(window,
            [](GLFWwindow* window, double xpos, double ypos)
            {
                callback_static(window, xpos, ypos);
            }
        );
        assert(old == nullptr);
    }
    if (handleMouseButtonEvents) {
        static auto callback_static = [this](GLFWwindow* window, int button, int action, int mods) {
            callbackMouseButton(window, button, action, mods);
            };
        auto old = glfwSetMouseButtonCallback(window,
            [](GLFWwindow* window, int button, int action, int mods)
            {
                callback_static(window, button, action, mods);
            }
        );
        assert(old == nullptr);
    }
}

void Presentation::destroyWindowResources(WindowInfo* wi)
{
    if (wi->swapChain != nullptr) {
        vkDestroySwapchainKHR(engine->globalRendering.device, wi->swapChain, nullptr);
        wi->swapChain = nullptr;
    }
    if (wi->surface != nullptr) {
        vkDestroySurfaceKHR(engine->globalRendering.vkInstance, wi->surface, nullptr);
        wi->surface = nullptr;
    }
    if (wi->glfw_window != nullptr) {
        glfwDestroyWindow(wi->glfw_window);
        wi->glfw_window = nullptr;
    }
    if (wi->imageAvailableSemaphore != nullptr) {
        vkDestroySemaphore(engine->globalRendering.device, wi->imageAvailableSemaphore, nullptr);
        wi->imageAvailableSemaphore = nullptr;
    }
    if (wi->renderFinishedSemaphore != nullptr) {
        vkDestroySemaphore(engine->globalRendering.device, wi->renderFinishedSemaphore, nullptr);
        wi->renderFinishedSemaphore = nullptr;
    }
    if (wi->inFlightFence != nullptr) {
        vkDestroyFence(engine->globalRendering.device, wi->inFlightFence, nullptr);
        wi->inFlightFence = nullptr;
    }
    if (wi->uiRenderFinished != nullptr) {
        vkDestroyEvent(engine->globalRendering.device, wi->uiRenderFinished, nullptr);
        wi->uiRenderFinished = nullptr;
    }
    if (wi->imageDumpFence != nullptr) {
        vkDestroyFence(engine->globalRendering.device, wi->imageDumpFence, nullptr);
        wi->imageDumpFence = nullptr;
    }
    if (wi->presentFence != nullptr) {
        vkDestroyFence(engine->globalRendering.device, wi->presentFence, nullptr);
        wi->presentFence = nullptr;
    }
}

void Presentation::callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    assert(engine->isMainThread());
    inputState.mouseButtonEvent = inputState.mouseMoveEvent = false;
    inputState.keyEvent = true;
    inputState.key = key;
    inputState.scancode = scancode;
    inputState.action = action;
    inputState.mods = mods;
    engine->app->handleInput(inputState);
}

void Presentation::callbackCursorPos(GLFWwindow* window, double xpos, double ypos)
{
    assert(engine->isMainThread());
    inputState.mouseButtonEvent = inputState.keyEvent = false;
    inputState.mouseMoveEvent = true;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    inputState.pos.x = static_cast<float>(xpos / width);
    inputState.pos.y = static_cast<float>(ypos / height);
    engine->app->handleInput(inputState);
}

void Presentation::callbackMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    assert(engine->isMainThread());
    inputState.mouseMoveEvent = inputState.keyEvent = false;
    inputState.mouseButtonEvent = true;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        inputState.pressedLeft = action == GLFW_PRESS;
        inputState.pressedRight = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        inputState.pressedRight = action == GLFW_PRESS;
        inputState.pressedLeft = false;
    }
    inputState.stillPressedLeft = false;
    inputState.stillPressedRight = false;
    // Check if the left mouse button is still pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        //std::cout << "Left mouse button is still pressed." << std::endl;
        inputState.stillPressedLeft = true;
    }
    // Check if the right mouse button is still pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        //std::cout << "Right mouse button is still pressed." << std::endl;
        inputState.stillPressedRight = true;
    }
    engine->app->handleInput(inputState);
}

void Presentation::pollEvents()
{
    if (isActive()) {
        ThemedTimer::getInstance()->add(TIMER_INPUT_THREAD);
        glfwPollEvents();
        if (glfwWindowShouldClose(windowInfo->glfw_window)) {
            // Handle window close event
            //Log("Window close button pressed\n");
            inputState.windowClosed = windowInfo;
            engine->app->handleInput(inputState);
        }
    }
}

void Presentation::createSwapChain(WindowInfo* winfo) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine->globalRendering.physicalDevice, winfo->surface);

    // list available modes
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, true);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, true);
    // select preferred mode
    surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    Log("swapchain format selected: " << surfaceFormat.format << endl);
    presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    Log("swapchain min max images: " << swapChainSupport.capabilities.minImageCount << " " << swapChainSupport.capabilities.maxImageCount << endl);
    winfo->imageCount = swapChainSupport.capabilities.minImageCount + 1;
    auto imageCount = winfo->imageCount;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = winfo->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = engine->globalRendering.findQueueFamilies(engine->globalRendering.physicalDevice);
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
    if (vkCreateSwapchainKHR(engine->globalRendering.device, &createInfo, nullptr, &winfo->swapChain) != VK_SUCCESS) {
        Error("failed to create swap chain!");
    }
    // retrieve swap chain images:
    vkGetSwapchainImagesKHR(engine->globalRendering.device, winfo->swapChain, &imageCount, nullptr);
    winfo->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(engine->globalRendering.device, winfo->swapChain, &imageCount, winfo->swapChainImages.data());
    assert(imageCount == winfo->imageCount);
    winfo->swapChainImageFormat = surfaceFormat.format;
    winfo->swapChainExtent = extent;
    Log("swap chain created with # images: " << imageCount << endl);
}

SwapChainSupportDetails Presentation::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
            Log("swapchain possible format: " << availableFormat.format << " color Space: " << availableFormat.colorSpace << endl);
        }
        else {
            if (availableFormat.format == GlobalRendering::ImageFormat && availableFormat.colorSpace == GlobalRendering::ImageColorSpace) {
                return availableFormat;
            }
        }
    }
    if (listmode) return availableFormats[0];
    Error("could not find right swapchain format");
    return availableFormats[0];
}

// list available modes or sleect preferred one
VkPresentModeKHR Presentation::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (listmode) {
            Log("swapchain possible present mode: " << availablePresentMode << endl);
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

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}



void Presentation::presentImage(WindowInfo* winfo)
{
    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    uint32_t imageIndex;
    if (vkAcquireNextImageKHR(device, winfo->swapChain, UINT64_MAX, winfo->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS) {
        Error("cannot aquire next image KHR");
    }
    //VkPresentInfoKHR presentInfo{};
    //presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    //presentInfo.waitSemaphoreCount = 1;
    //presentInfo.pWaitSemaphores = signalSemaphores;

    //VkSwapchainKHR swapChains[] = { swapChain };
    //presentInfo.swapchainCount = 1;
    //presentInfo.pSwapchains = swapChains;
    //presentInfo.pImageIndices = &imageIndex;
    //presentInfo.pResults = nullptr; // Optional
    //vkQueuePresentKHR(winfo->presentQueue, &presentInfo);
    ////LogF("Frame presented: " << tr.frameNum << endl);
}

void Presentation::preparePresentation(WindowInfo* winfo)
{

    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    createSwapChain(winfo);
    auto v = global.familyIndices.presentFamily.value();
    vkGetDeviceQueue(device, v, 0, &winfo->presentQueue);
    //engine.global.presentQueueFamiliyIndex = value;
    //engine.global.presentQueueIndex = 0;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = global.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &winfo->commandBufferPresentBack) != VK_SUCCESS) {
        Error("failed to allocate command buffers!");
    }
    winfo->commandBufferDebugName = "window.commandBufferPresentBack";
    engine->util.debugNameObjectCommandBuffer(winfo->commandBufferPresentBack, winfo->commandBufferDebugName.c_str());
    if (vkAllocateCommandBuffers(device, &allocInfo, &winfo->commandBufferUI) != VK_SUCCESS) {
        Error("failed to allocate command buffers!");
    }

    // semaphores and fences
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &winfo->imageAvailableSemaphore) != VK_SUCCESS) {
        Error("failed to create imageAvailableSemaphore for a frame");
    }
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &winfo->renderFinishedSemaphore) != VK_SUCCESS) {
        Error("failed to create renderFinishedSemaphore for a frame");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    if (vkCreateFence(device, &fenceInfo, nullptr, &winfo->imageDumpFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    //fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(device, &fenceInfo, nullptr, &winfo->presentFence) != VK_SUCCESS) {
        Error("failed to create presentFence for a frame");
    }
    fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
    if (vkCreateFence(device, &fenceInfo, nullptr, &winfo->inFlightFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }

    VkEventCreateInfo eventInfo{};
    eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    eventInfo.flags = 0;
    if (vkCreateEvent(global.device, &eventInfo, nullptr, &winfo->uiRenderFinished) != VK_SUCCESS) {
        Error("failed to create event uiRenderFinished for a frame");
    }
}
