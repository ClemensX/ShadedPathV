#pragma once

// forward
struct ShaderState;

// #undefine for disabling the debug extension
#define ENABLE_DEBUG_MARKER_EXTENSION

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

	// select vulkan profile and API version
	VpProfileProperties profile{
			VP_KHR_ROADMAP_2022_NAME,
			VP_KHR_ROADMAP_2022_SPEC_VERSION
	};
	//uint32_t API_VERSION = VP_KHR_ROADMAP_2022_MIN_API_VERSION;

	// list device and instance extensions
	static const bool USE_PROFILE_DYN_RENDERING = false;

	// flag for debug marker extension used
#if defined(ENABLE_DEBUG_MARKER_EXTENSION)
	static const bool DEBUG_MARKER_EXTENSION = true;
#else
	static const bool DEBUG_MARKER_EXTENSION = false;
#endif
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

	// log some important device limits e.g. for buffer size and alignemnt 
	void logDeviceLimits();

	void createLogicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, bool listmode = false);
	QueueFamilyIndices familyIndices;
	uint32_t findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
	uint32_t presentQueueFamiliyIndex = -1;
	uint32_t presentQueueIndex = -1;

	// Vulkan helper

	// Upload index or vertex buffer
	void uploadBuffer(VkBufferUsageFlagBits usage, VkDeviceSize bufferSize, const void *src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	// Buffer Creation
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	// copy buffer
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	// images
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkImageView createImageViewCube(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
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
	inline uint64_t calcConstantBufferSize(uint64_t originalSize) {
		VkDeviceSize align = physicalDeviceProperties.properties.limits.minUniformBufferOffsetAlignment;
		return (originalSize + (align - 1)) & ~(align - 1); // must be a multiple of align bytes
	};

private:
	vector<const char*> deviceExtensions = {
		//VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
		//VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
#if defined(ENABLE_DEBUG_MARKER_EXTENSION)
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME
#endif
	};

	void gatherDeviceExtensions();
	void initVulkanInstance();

	vector<const char*> getRequiredExtensions();

	// devices
	void pickPhysicalDevice(bool listmode = false);
	// list or select physical devices
	bool isDeviceSuitable(VkPhysicalDevice device, bool listmode = false);

	// swap chain query
	bool checkDeviceExtensionSupport(VkPhysicalDevice phys_device, bool listmode);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	// check if selected profile is supported, return false otherwise
	bool checkProfileSupport() {
		VkResult result = VK_SUCCESS;
		VkBool32 supported = VK_FALSE;
		result = vpGetInstanceProfileSupport(nullptr, &profile, &supported);
		if (result != VK_SUCCESS) {
			// something went wrong
			Log("Vulkan profile not supported: " << profile.profileName << endl);
			return false;
		}
		else if (supported != VK_TRUE) {
			// profile is not supported at the instance level
			Log("Vulkan profile instance not supported: " << profile.profileName << " instance: " << profile.specVersion << endl);
			return false;
		}
		Log("Vulkan profile checked ok: " << profile.profileName << " instance: " << profile.specVersion << endl)
			return true;
	}
	bool checkDeviceProfileSupport(VkInstance instance, VkPhysicalDevice physDevice) {
		VkResult result = VK_SUCCESS;
		VkBool32 supported = VK_FALSE;
		result = vpGetPhysicalDeviceProfileSupport(instance, physDevice, &profile, &supported);
		if (result != VK_SUCCESS) {
			// something went wrong
			Log("Vulkan device profile not supported: " << profile.profileName << endl);
			return false;
		}
		else if (supported != VK_TRUE) {
			// profile is not supported at the instance level
			Log("Vulkan device profile not supported: " << profile.profileName << " instance: " << profile.specVersion << endl);
			return false;
		}
		Log("Vulkan device profile checked ok: " << profile.profileName << " instance: " << profile.specVersion << endl)
			return true;
	}
};

