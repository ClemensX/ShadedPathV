#pragma once

// forward
struct ShaderState;

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
	};

	~GlobalRendering() {
		Log("GlobalRendering destructor\n");
		shutdown();
	};

	// list device and instance extensions
	static const bool LIST_EXTENSIONS = false;

	// Vulkan formats we want to set centrally:

	static const VkFormat ImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	//static const VkFormat ImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	static const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
	//static const VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	static const VkColorSpaceKHR ImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	// initialize all global Vulkan stuff - engine configuration settings
	// cannot be changed after calling this, because some settings influence Vulkan creation options
	void initBeforePresentation();
	// device selection and creation
	void initAfterPresentation();

	// destroy global resources, should only be called from engine dtor
	void shutdown();

	void createLogicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, bool listmode = false);
	QueueFamilyIndices familyIndices;
	uint32_t findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

	// Vulkan helper

	// Upload index or vertex buffer
	void uploadBuffer(VkBufferUsageFlagBits usage, VkDeviceSize bufferSize, const void *src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	// Buffer Creation
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	// copy buffer
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	// images
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	// fill in viewport and scissor and create VkPipelineViewportStateCreateInfo with them
	void createViewportState(ShaderState &shaderState);
	// Vulkan entities
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = nullptr;
	VkInstance vkInstance = nullptr;
	VkQueue graphicsQueue = nullptr;
	VkSampler textureSampler = nullptr;
	VkPhysicalDeviceProperties2 physicalDeviceProperties;

	// create command pool for use outside rendering threads
	void createCommandPool();
	VkCommandPool commandPool = nullptr;
	void createCommandPool(VkCommandPool& pool);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createTextureSampler();

private:
	vector<const char*> deviceExtensions = {
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
	};

	void gatherDeviceExtensions();
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
	// list or select physical devices
	bool isDeviceSuitable(VkPhysicalDevice device, bool listmode = false);

	// swap chain query
	bool checkDeviceExtensionSupport(VkPhysicalDevice phys_device, bool listmode);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
};

