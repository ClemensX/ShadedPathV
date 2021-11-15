#pragma once

struct QueueFamilyIndices {
    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentFamily;
    bool isComplete(bool presentationEnabled) {
        if (presentationEnabled)
            return graphicsFamily.has_value() && presentFamily.has_value();
        else
            return graphicsFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class GlobalRendering;

// Engine initialization and global object that are not shader specific
// like framebuffer, swap chain and render passes
class ShadedPathEngine
{
public:
    // construct engine instance together with its needed aggregates
    ShadedPathEngine() :
        global(*this)
    {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine()
    {
        Log("Engine destructor\n");
        vkDeviceWaitIdle(device);
        threadResources.clear();
        global.destroy();
        shutdown();
    };

    // prevent copy and assigment
    //ShadedPathEngine(ShadedPathEngine const&) = delete;
    //void operator=(ShadedPathEngine const&) = delete;

    // initialize Vulkan
    void init();
    // exit Vulkan and free resources
    void shutdown();
    // enable output window, withour calling this only background processing is possible
    void enablePresentation(int w, int h, const char* name) {
        if (vkInstance) {
            Error("Changing presentation mode after initialization is not possible!");
        }
        win_width = w;
        win_height = h;
        win_name = name;
        currentExtent.width = w;
        currentExtent.height = h;
        presentationEnabled = true;
    };

    // set number of frames that can be worked on in parallel
    // default is 2
    void setFramesInFlight(int n) {
        framesInFlight = n;
        threadResources.resize(framesInFlight);
    }
    // current frame index - always within 0 .. threadResources.size() - 1
    size_t currentFrame = 0;

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    // call render cod in shaders for one frame
    void drawFrame();

    // we need a method to get current extent that work for presentation mode and without swap chain
    VkExtent2D getCurrentExtent();
    GLFWwindow* window = nullptr;
    VkDevice device = nullptr;
    VkRenderPass renderPass;
    // non-Vulkan members
    Files files;
    GlobalRendering global;
    vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    vector<VkCommandBuffer> commandBuffers;
    VkSwapchainKHR swapChain{};
    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    vector<ThreadResources> threadResources;
    GameTime gameTime;

private:
    int framesInFlight = 2;
    bool presentationEnabled = false;
    VkInstance vkInstance = nullptr;
    VkSurfaceKHR surface = nullptr;
    QueueFamilyIndices familyIndices;
    vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
    vector<VkImageView> swapChainImageViews;
    // initialization
    void initGLFW();
    void initVulkanInstance();
    // validation layer
    VkDebugUtilsMessengerEXT debugMessenger = nullptr;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    bool checkValidationLayerSupport();
    vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

    // devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // list or select physical devices
    void pickPhysicalDevice(bool listmode = false);
    bool isDeviceSuitable(VkPhysicalDevice device, bool listmode = false);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();

    // presentation
    void createSurface();
    int win_width = 0;
    int win_height = 0;
    const char* win_name = nullptr;
    // if no window or backbuffer size was set by application:
    VkExtent2D defaultExtent = { 500, 400 };
    VkExtent2D currentExtent = defaultExtent;

    // swap chain
    bool checkDeviceExtensionSupport(VkPhysicalDevice phys_device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    // choose swap chain format or list available formats
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, bool listmode = false);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode = false);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    // swap chain image views
    void createImageViews();

    // render pass
    void createRenderPass();

    // frame buffers
    void createFramebuffers();

    // command pool
    void createCommandPool();

    // command buffers
    void createCommandBuffers();
};
