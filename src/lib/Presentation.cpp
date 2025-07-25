#include "mainheader.h"

using namespace std;

void glfwErrorCallback(int error, const char* description) {
    Log("GLFW Error (" << error << "): " << description << endl);
    //cerr << "GLFW Error (" << error << "): " << description << endl;
}

// Initialize the static member function pointers
// the will be reset every time a new Presentation object is created (for another engine run)
std::function<void(GLFWwindow*, int, int, int, int)> Presentation::currentKeyCallback = nullptr;
std::function<void(GLFWwindow* window, int button, int action, int mods)> Presentation::currentMouseButtonCallback = nullptr;
std::function<void(GLFWwindow* window, double x, double y)> Presentation::currentCursorPosCallback = nullptr;

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
    for (auto& gpui : gpuImages) {
        if (gpui.fba.image != nullptr) {
            vkDestroyImageView(engine->globalRendering.device, gpui.fba.view, nullptr);
            vkDestroyFramebuffer(engine->globalRendering.device, gpui.framebuffer, nullptr);
        }
    }
}

// handle key callback
void customKeyCallback(Presentation* presentation, GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Custom key handling logic
    //presentation->keyCallback(window, key, scancode, action, mods);
    presentation->callbackKey(window, key, scancode, action, mods);
}

void Presentation::setKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> callback) {
    currentKeyCallback = callback;
}

void Presentation::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (currentKeyCallback) {
        currentKeyCallback(window, key, scancode, action, mods);
    }
}

// handle MousButton callback
void customMouseButtonCallback(Presentation* presentation, GLFWwindow* window, int button, int action, int mods) {
    // Custom key handling logic
    presentation->callbackMouseButton(window, button, action, mods);
}

void Presentation::setMouseButtonCallback(std::function<void(GLFWwindow* window, int button, int action, int mods)> callback) {
    currentMouseButtonCallback = callback;
}

void Presentation::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (currentMouseButtonCallback) {
        currentMouseButtonCallback(window, button, action, mods);
    }
}

// handle key callback
void customCursorPosCallback(Presentation* presentation, GLFWwindow* window, double x, double y) {
    // Custom key handling logic
    presentation->callbackCursorPos(window, x, y);
}

void Presentation::setCursorPosCallback(std::function<void(GLFWwindow* window, double x, double y)> callback) {
    currentCursorPosCallback = callback;
}

void Presentation::cursorPosCallback(GLFWwindow* window, double x, double y) {
    if (currentCursorPosCallback) {
        currentCursorPosCallback(window, x, y);
    }
}

void Presentation::initializeCallbacks(GLFWwindow* window, bool handleKeyEvents, bool handleMouseMoveEvents, bool handleMouseButtonEvents) {
    // init callbacks: we assume that no other callback was installed,
    // or was already removed
    if (handleKeyEvents) {
        setKeyCallback([this](GLFWwindow* window, int key, int scancode, int action, int mods) {
            customKeyCallback(this, window, key, scancode, action, mods);
            });
        glfwSetKeyCallback(window, keyCallback);
    }
    if (handleMouseButtonEvents) {
        setMouseButtonCallback([this](GLFWwindow* window, int button, int action, int mods) {
            customMouseButtonCallback(this, window, button, action, mods);
            });
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }
    if (handleMouseMoveEvents) {
        setCursorPosCallback([this](GLFWwindow* window, double x, double y) {
            customCursorPosCallback(this, window, x, y);
            });
        glfwSetCursorPosCallback(window, cursorPosCallback);
    }
}

void Presentation::detachFromWindow(WindowInfo* wi, ContinuationInfo* contInfo)
{
    if (wi->swapChain != nullptr) {
        vkDestroySwapchainKHR(engine->globalRendering.device, wi->swapChain, nullptr);
        wi->swapChain = nullptr;
    }
    if (wi->surface != nullptr) {
        vkDestroySurfaceKHR(engine->globalRendering.vkInstance, wi->surface, nullptr);
        wi->surface = nullptr;
    }
    glfwSetKeyCallback(wi->glfw_window, nullptr);
    glfwSetMouseButtonCallback(wi->glfw_window, nullptr);
    glfwSetCursorPosCallback(wi->glfw_window, nullptr);

    contInfo->windowInfo = *wi;
    endPresentation(wi);
}

void Presentation::reuseWindow(WindowInfo* wi, ContinuationInfo* contInfo, bool handleKeyEvents, bool handleMouseMoveEevents, bool handleMouseButtonEvents)
{
    if (contInfo->windowInfo.glfw_window == nullptr) {
        Error("cannot reuse window without glfw window");
    }
    Log("reuse window\n");
    wi->glfw_window = contInfo->windowInfo.glfw_window;
    wi->title = contInfo->windowInfo.title;
    wi->width = contInfo->windowInfo.width;
    wi->height = contInfo->windowInfo.height;
    windowInfo = wi;
    if (glfwCreateWindowSurface(engine->globalRendering.vkInstance, wi->glfw_window, nullptr, &wi->surface) != VK_SUCCESS) {
        Error("failed to create window surface!");
    }
    initializeCallbacks(wi->glfw_window, handleKeyEvents, handleMouseMoveEevents, handleMouseButtonEvents);
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
    winfo->width = w;
    winfo->height = h;
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
    initializeCallbacks(window, handleKeyEvents, handleMouseMoveEevents, handleMouseButtonEvents);
}

void Presentation::startUI()
{
    if (!engine->isEnableUI()) return;
    engine->ui.enable();
    //engine->shaders.uiShader.enabled = true;
    engine->shaders.uiShader.init(*engine, engine->shaders.getShaderState());
}

void Presentation::destroyWindowResources(WindowInfo* wi, bool destroyGlfwWindow)
{
    if (wi->swapChain != nullptr) {
        vkDestroySwapchainKHR(engine->globalRendering.device, wi->swapChain, nullptr);
        wi->swapChain = nullptr;
    }
    if (wi->surface != nullptr) {
        vkDestroySurfaceKHR(engine->globalRendering.vkInstance, wi->surface, nullptr);
        wi->surface = nullptr;
    }
    if (wi->glfw_window != nullptr && destroyGlfwWindow) {
        glfwDestroyWindow(wi->glfw_window);
        wi->glfw_window = nullptr;
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
    if (wi->imageAvailableSemaphore != nullptr) {
        vkDestroySemaphore(engine->globalRendering.device, wi->imageAvailableSemaphore, nullptr);
        wi->imageAvailableSemaphore = nullptr;
    }
    if (wi->renderFinishedSemaphore != nullptr) {
        vkDestroySemaphore(engine->globalRendering.device, wi->renderFinishedSemaphore, nullptr);
        wi->renderFinishedSemaphore = nullptr;
    }
    if (wi->prePresentCompleteSemaphore != nullptr) {
        vkDestroySemaphore(engine->globalRendering.device, wi->prePresentCompleteSemaphore, nullptr);
        wi->prePresentCompleteSemaphore = nullptr;
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
    for (int i = 0; i < winfo->swapChainImages.size(); i++) {
        auto& image = winfo->swapChainImages[i];
        engine->util.debugNameObjectImage(image, engine->util.createDebugName("swapchain image", i).c_str());
    }
    Log("swap chain created with # images: " << imageCount << endl);
}

void Presentation::createPresentQueue(unsigned int value)
{
    //vkGetDeviceQueue(engine.global.device, value, 0, &presentQueue);
    engine->globalRendering.presentQueueFamiliyIndex = value;
    engine->globalRendering.presentQueueIndex = 0;
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

GPUImage& Presentation::getCreateGPUImage(uint32_t index, WindowInfo* winfo)
{
    if (index < gpuImages.size()) {
        return gpuImages[index];
    }
    gpuImages.resize(index + 1);
    GPUImage& dstImage = gpuImages[index];
    dstImage.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    dstImage.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    dstImage.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    dstImage.fba.image = winfo->swapChainImages[index];
    dstImage.fba.view = engine->globalRendering.createImageView(dstImage.fba.image, winfo->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    return dstImage;
}

void Presentation::presentImage(FrameResources* fr, WindowInfo* winfo)
{
    // 1. step: aquire image, wait for aquire semaphore, then create the copy commands and execute them
    if (winfo->disabled) return;
    if (winfo->glfw_window == nullptr) Error("no window available");
    auto& device = engine->globalRendering.device;
    auto& global = engine->globalRendering;
    uint32_t imageIndex;

    if (vkAcquireNextImageKHR(device, winfo->swapChain, UINT64_MAX, winfo->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS) {
        Error("cannot aquire next image KHR");
    }
    VkSemaphoreSubmitInfo acquireCompleteInfo = {};
    acquireCompleteInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    acquireCompleteInfo.semaphore = winfo->imageAvailableSemaphore;
    acquireCompleteInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBufferSubmitInfo renderingCommandBufferInfo = {};
    renderingCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    renderingCommandBufferInfo.commandBuffer = winfo->commandBufferPresentBack;


    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    // UI code
    engine->shaders.uiShader.draw(fr, winfo, &fr->colorImage);
    
    if (vkBeginCommandBuffer(winfo->commandBufferPresentBack, &beginInfo) != VK_SUCCESS) {
        Error("failed to begin recording back buffer copy command buffer!");
    }
    // Transition image formats

    // set src values for access, layout and image
    //GPUImage dstImage{};
    //dstImage.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    //dstImage.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dstImage.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    //dstImage.fba.image = winfo->swapChainImages[imageIndex];
    auto& dstImage = getCreateGPUImage(imageIndex, winfo);
    DirectImage::toLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, winfo->commandBufferPresentBack, &dstImage);
    if (engine->shaders.uiShader.enabled) {
        // NEW
        //DirectImage::toLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, winfo->commandBufferPresentBack, &fr->colorImage);
    }
    DirectImage::toLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, winfo->commandBufferPresentBack, &fr->colorImage);

    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSizeSrc;
    blitSizeSrc.x = engine->getBackBufferExtent().width;
    blitSizeSrc.y = engine->getBackBufferExtent().height;
    blitSizeSrc.z = 1;
    VkOffset3D blitSizeDst;
    blitSizeDst.x = winfo->width;
    if (engine->isStereoPresentation()) {
        blitSizeDst.x = winfo->width / 2;
    }
    blitSizeDst.y = winfo->height;
    blitSizeDst.z = 1;

    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSizeSrc;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSizeDst;

    if (engine->isVR()) {
#if defined(OPENXR_AVAILABLE)
        engine->vr.frameCopy(*fr, winfo);
#endif
    }

    vkCmdBlitImage(
        winfo->commandBufferPresentBack,
        fr->colorImage.fba.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage.fba.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageBlitRegion,
        VK_FILTER_LINEAR
    );

    // right eye:
    if (engine->isStereoPresentation()) {
        DirectImage::toLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, winfo->commandBufferPresentBack, &fr->colorImage2);
        VkOffset3D blitPosDst;
        blitPosDst.x = winfo->width / 2;
        blitPosDst.y = 0;
        blitPosDst.z = 0;
        imageBlitRegion.dstOffsets[0] = blitPosDst;
        blitSizeDst.x = winfo->width;
        imageBlitRegion.dstOffsets[1] = blitSizeDst;
        vkCmdBlitImage(
            winfo->commandBufferPresentBack,
            fr->colorImage2.fba.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage.fba.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageBlitRegion,
            VK_FILTER_LINEAR
        );
    }

    if (engine->shaders.uiShader.enabled) {
        // NEW
        // acquireCompleteInfo.semaphore = fr->imageAvailableSemaphore;
        engine->ui.update();
        DirectImage::toLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, winfo->commandBufferPresentBack, &dstImage);
        if (dstImage.framebuffer == nullptr) {
            engine->shaders.uiShader.initFramebuffer(dstImage.fba.view, dstImage.framebuffer);
        }
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = engine->ui.imGuiRenderPass;//tr.renderPassDraw;
        renderPassInfo.framebuffer = dstImage.framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        VkExtent2D extent{(uint32_t)winfo->width, (uint32_t)winfo->height};
        renderPassInfo.renderArea.extent = extent;//engine->getBackBufferExtent();
        renderPassInfo.clearValueCount = 0;
        vkCmdBeginRenderPass(winfo->commandBufferPresentBack, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), winfo->commandBufferPresentBack);
        vkCmdEndRenderPass(winfo->commandBufferPresentBack);
    }
    DirectImage::toLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_MEMORY_READ_BIT, winfo->commandBufferPresentBack, &dstImage);

    if (vkEndCommandBuffer(winfo->commandBufferPresentBack) != VK_SUCCESS) {
        Error("failed to record back buffer copy command buffer!");
    }

    VkSubmitInfo2 renderingSubmitInfo{};
    renderingSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    renderingSubmitInfo.waitSemaphoreInfoCount = 1;
    renderingSubmitInfo.pWaitSemaphoreInfos = &acquireCompleteInfo;
    renderingSubmitInfo.commandBufferInfoCount = 1;
    renderingSubmitInfo.pCommandBufferInfos = &renderingCommandBufferInfo;

    if (vkQueueSubmit2(global.graphicsQueue, 1, &renderingSubmitInfo, winfo->presentFence) != VK_SUCCESS) {
        Error("failed to submit draw command buffer!");
    }
    vkWaitForFences(global.device, 1, &winfo->presentFence, VK_TRUE, UINT64_MAX);
    vkResetFences(global.device, 1, &winfo->presentFence);
    //vkQueueWaitIdle(global.graphicsQueue);

    if (engine->isVR()) {
#if defined(OPENXR_AVAILABLE)
        engine->vr.frameEnd(*fr);
#endif
    }
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores = &winfo->renderFinishedSemaphore;//&winfo->prePresentCompleteSemaphore;

    VkSwapchainKHR swapChains[] = { winfo->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(winfo->presentQueue, &presentInfo);
    //LogF("Frame presented: " << tr.frameNum << endl);
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
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &winfo->prePresentCompleteSemaphore) != VK_SUCCESS) {
        Error("failed to create prePresentCompleteSemaphore for a frame");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    if (vkCreateFence(device, &fenceInfo, nullptr, &winfo->imageDumpFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    fenceInfo.flags = 0; // present fence will be set during 1st present in queue submit thread
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

void Presentation::endPresentation(WindowInfo* winfo)
{
    destroyWindowResources(winfo, false);
}
