#pragma once

struct QueueFamilyIndices {
    optional<uint32_t> graphicsFamily;
    bool isComplete() {
        return graphicsFamily.has_value();
    }
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
private:
    GLFWwindow* window = nullptr;
    VkInstance vkInstance;
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
};

