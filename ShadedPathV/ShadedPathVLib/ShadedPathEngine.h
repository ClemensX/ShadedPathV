#pragma once

struct QueueFamilyIndices {
    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentFamily;
};

class ShadedPathEngine
{
public:
    ShadedPathEngine() {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine() {
        Log("Engine destructor\n");
    };

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
        presentationEnabled = true;
    };
    GLFWwindow* window = nullptr;
private:
    bool presentationEnabled = false;
    VkInstance vkInstance = nullptr;
    VkDevice device = nullptr;
    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    VkSurfaceKHR surface = nullptr;
    QueueFamilyIndices familyIndices;
    // validation layer
    VkDebugUtilsMessengerEXT debugMessenger;
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
    bool isCompleteFamilyIndices() {
        if (presentationEnabled)
            return familyIndices.graphicsFamily.has_value() && familyIndices.presentFamily.has_value();
        else
            return familyIndices.graphicsFamily.has_value();
    }

    // presentation
    void createSurface();
    int win_width;
    int win_height;
    const char* win_name;
};

