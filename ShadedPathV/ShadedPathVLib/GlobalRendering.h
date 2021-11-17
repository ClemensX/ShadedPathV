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

// forward declarations
class ShadedPathEngine;

// global resources that are not changed in rendering threads.
// shader code, meshes, etc.
// all objects here are specific to shaders
// shader independent global objects like framebuffer, swap chain, render passes are in ShadedPathEngine
class GlobalRendering
{
private:
	// we need direct access to engine instance
	ShadedPathEngine& engine;

public:
	GlobalRendering(ShadedPathEngine& s) : engine(s) {
		Log("GlobalRendering c'tor\n");
		init();
	};
	~GlobalRendering() {
		Log("GlobalRendering destructor\n");
	};
	// detroy global resources, should only be called from engine dtor
	void destroy();
	Files files;

	// Vulkan 
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = nullptr;

private:
	void init();
	void initVulkanInstance();
	void setupDebugMessenger();

	// validation layer
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	bool checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	// devices
	void pickPhysicalDevice(bool listmode = false);
	void createLogicalDevice();
	QueueFamilyIndices familyIndices;
	// list or select physical devices
	bool isDeviceSuitable(VkPhysicalDevice device, bool listmode = false);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	// swap chain query
	bool checkDeviceExtensionSupport(VkPhysicalDevice phys_device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkInstance vkInstance = nullptr;
};

