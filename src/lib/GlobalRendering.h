#pragma once

// forward
struct ShaderState;

// #undefine for disabling the debug extension
#if defined(DEBUG) | defined(_DEBUG)
#define ENABLE_DEBUG_UTILS_EXTENSION
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete(bool presentationEnabled) {
		if (presentationEnabled)
			return graphicsFamily.has_value() && presentFamily.has_value();
		else
			return graphicsFamily.has_value();
	}
};

// define what needs to be transferred to main threads for submitting updates from update thread
struct SingleQueueTransferInfo {
	VkSubmitInfo* submitInfoAddr;
};

// global resources that are not changed in rendering threads.
class GlobalRendering : public EngineParticipant
{
public:
	GlobalRendering(ShadedPathEngine* s) {
		Log("GlobalRendering c'tor\n");
        setEngine(s);
		// log vulkan version as string
		Log("Vulkan API Version: " << getVulkanAPIString().c_str() << std::endl);
	};

	~GlobalRendering() {
		Log("GlobalRendering destructor\n");
		shutdown();
	};

	// select vulkan profile and API version
	VpProfileProperties profile{
#		if defined(USE_SMALL_GRAPHICS)
			VP_KHR_ROADMAP_2022_NAME,
			VP_KHR_ROADMAP_2022_SPEC_VERSION
#		else
			//VP_LUNARG_DESKTOP_PORTABILITY_2021_NAME,
			//VP_LUNARG_DESKTOP_PORTABILITY_2021_SPEC_VERSION
			//VP_LUNARG_DESKTOP_BASELINE_2022_NAME,
			//VP_LUNARG_DESKTOP_BASELINE_2022_SPEC_VERSION
			VP_KHR_ROADMAP_2024_NAME,
			VP_KHR_ROADMAP_2024_SPEC_VERSION
#		endif
	};
	//uint32_t API_VERSION = VP_KHR_ROADMAP_2022_MIN_API_VERSION;

	static const bool USE_PROFILE_DYN_RENDERING = false;

	// flag for debug marker extension used
#if defined(ENABLE_DEBUG_UTILS_EXTENSION)
	static const bool DEBUG_UTILS_EXTENSION = true;
#else
	static const bool DEBUG_UTILS_EXTENSION = false;
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
	VkSurfaceKHR surface = nullptr;

	// Vulkan helper

	enum class QueueSelector { GRAPHICS, TRANSFER };
	static const uint64_t QUEUE_FLAG_PERMANENT_UPDATE = 0x01L;
	// Upload index or vertex buffer
	void uploadBuffer(VkBufferUsageFlagBits usage, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory,
		std::string bufferDebugName, QueueSelector queue = QueueSelector::GRAPHICS, uint64_t flags = 0L );
	// Buffer Creation
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, std::string bufferDebugName);
	// copy buffer
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, QueueSelector queue = QueueSelector::GRAPHICS, uint64_t flags = 0L);
	// images
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkImageView createImageViewCube(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	// create image and bound memory with default parameters - use layers == 6 for cube map
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t layers = 1);
	// create cube image and bound memory with default parameters
	void createImageCube(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		createImage(width, height, mipLevels, numSamples, format, tiling, usage, properties, image, imageMemory, 6);
	}
	void createCubeMapFrom2dTexture(std::string textureName2d, std::string textureNameCube, TextureStore* textureStore);
	void destroyImage(VkImage image, VkDeviceMemory imageMemory);
	void destroyImageView(VkImageView imageView);

	// fill in viewport and scissor and create VkPipelineViewportStateCreateInfo with them
	void createViewportState(ShaderState &shaderState);
	// Vulkan entities
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = nullptr;
	VkInstance vkInstance = nullptr;
	VkQueue graphicsQueue = nullptr;
	VkQueue transferQueue = nullptr;
	VkSampler textureSampler[(int)TextureType::TEXTURE_TYPE_COUNT];
	VkPhysicalDeviceProperties2 physicalDeviceProperties;
	VkSemaphore singleTimeCommandsSemaphore = nullptr;
	// create command pool for use outside rendering threads

	void createCommandPools();
	VkCommandPool commandPool = nullptr;
	VkCommandPool commandPoolTransfer = nullptr;
	bool syncedOperations = false;
	VkCommandBuffer commandBufferSingle = nullptr;
	void createCommandPool(VkCommandPool& pool);
	void createCommandPoolTransfer(VkCommandPool& pool);
	// single time commands with optional syncing
	VkCommandBuffer beginSingleTimeCommands(bool sync = false, QueueSelector queue = QueueSelector::GRAPHICS);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, bool sync = false, QueueSelector queue = QueueSelector::GRAPHICS, uint64_t flags = 0L);
	void createTextureSampler();
	inline uint64_t calcConstantBufferSize(uint64_t originalSize) {
		VkDeviceSize align = physicalDeviceProperties.properties.limits.minUniformBufferOffsetAlignment;
		return (originalSize + (align - 1)) & ~(align - 1); // must be a multiple of align bytes
	};
	static std::string getVulkanAPIString();

	// AI method to return current time as UTC string
    std::string getCurrentTimeUTCString() {
        // Get the current time
        std::time_t currentTime = std::time(nullptr);

        // Convert the current time to UTC string
        std::tm* utcTime = std::gmtime(&currentTime);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utcTime);

        return std::string(buffer);
    }

	// low level graphics

	// create image in GPU with default settings (render target, no mipmaps, not host visible)
	GPUImage* createImage(std::vector<GPUImage>& list, const char* debugName, uint32_t width = 0, uint32_t height = 0);
    // create image in GPU with direct memory access by host, can be used for dumping to file or CPU based image manipulation
    void createDumpImage(GPUImage& gpui, uint32_t width = 0, uint32_t height = 0);
	void destroyImage(GPUImage* image);

private:
	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME
		//VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
		//VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
		//VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
		// debug_utils is only an instance extension
//#if defined(ENABLE_DEBUG_UTILS_EXTENSION)
//		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
//#endif
	};

	void gatherDeviceExtensions();
	void initVulkanInstance();

	std::vector<const char*> getRequiredExtensions();

	// devices
	void assignGlobals(VkPhysicalDevice phys);
	void pickPhysicalDevice(bool listmode = false);
	// list or select physical devices
	bool isDeviceSuitable(VkPhysicalDevice device, bool listmode = false);
	// list queue flags as text
	std::string getQueueFlagsString(VkQueueFlags flags);

	// swap chain query
	bool checkDeviceExtensionSupport(VkPhysicalDevice phys_device, bool listmode);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	// check if selected profile is supported, return false otherwise
	bool checkProfileSupport() {
		return true;
		VkResult result = VK_SUCCESS;
		VkBool32 supported = VK_FALSE;
		result = vpGetInstanceProfileSupport(nullptr, &profile, &supported);
		if (result != VK_SUCCESS) {
			// something went wrong
			Log("Vulkan profile not supported: " << profile.profileName << std::endl);
			return false;
		}
		else if (supported != VK_TRUE) {
			// profile is not supported at the instance level
			Log("Vulkan profile instance not supported: " << profile.profileName << " instance: " << profile.specVersion << std::endl);
			return false;
		}
		Log("Vulkan profile checked ok: " << profile.profileName << " instance: " << profile.specVersion << std::endl)
			return true;
	}
	bool checkDeviceProfileSupport(VkInstance instance, VkPhysicalDevice physDevice) {
		VkResult result = VK_SUCCESS;
		VkBool32 supported = VK_FALSE;
		result = vpGetPhysicalDeviceProfileSupport(instance, physDevice, &profile, &supported);
		if (result != VK_SUCCESS) {
			// something went wrong
			Log("Vulkan device profile not supported: " << profile.profileName << std::endl);
			return false;
		}
		else if (supported != VK_TRUE) {
			// profile is not supported at the instance level
			Log("Vulkan device profile not supported: " << profile.profileName << " instance: " << profile.specVersion << std::endl);
			return false;
		}
		Log("Vulkan device profile checked ok: " << profile.profileName << " instance: " << profile.specVersion << std::endl)
			return true;
	}
};

