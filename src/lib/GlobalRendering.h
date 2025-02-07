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

	std::vector<const char*> instanceExtensions = {
#   if defined(ENABLE_DEBUG_UTILS_EXTENSION)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#   endif
#   if defined(__APPLE__)
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#   endif
#   if defined(_WIN64)
        "VK_KHR_win32_surface",
#   endif
		VK_KHR_SURFACE_EXTENSION_NAME
	};

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_MESH_SHADER_EXTENSION_NAME//,
		//VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
	};

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
	void init();

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
    VkSurfaceKHR surface = nullptr; // TODO not initialized before window creation, need to find a way to NOT use this

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
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, const char* debugName, uint32_t layers = 1);
	// create cube image and bound memory with default parameters
	void createImageCube(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, const char* debugName) {
		createImage(width, height, mipLevels, numSamples, format, tiling, usage, properties, image, imageMemory, debugName, 6);
	}
	void createCubeMapFrom2dTexture(std::string textureName2d, std::string textureNameCube, TextureStore* textureStore);
	void destroyImage(VkImage image, VkDeviceMemory imageMemory);
	void destroyImageView(VkImageView imageView);

	// fill in viewport and scissor and create VkPipelineViewportStateCreateInfo with them
	void createViewportState(ShaderState &shaderState);
	// Vulkan entities
	VkPhysicalDevice physicalDevice = nullptr;
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
    std::vector<ThreadResources> workerThreadResources;
	bool syncedOperations = false;
	VkCommandBuffer commandBufferSingle = nullptr;
	void createCommandPool(VkCommandPool& pool, std::string name = "");
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
	bool isDeviceExtensionRequested(const char* extensionName) {
        return isExtensionAvailable(deviceExtensions, extensionName);
	}
	bool isMeshShading() {
        return isDeviceExtensionRequested(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	}
    bool isSynchronization2() {
        return isDeviceExtensionRequested(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    }

    // dump a rendered image to file
    void dumpToFile(FrameBufferAttachment* fba);

    // submit command buffers, can only be called from queue submit thread
    void submit(FrameResources* fr);
private:
    // gather all cmd buffers from the DrawResults of the current frame and copy into single list cmdBufs
	void consolidateCommandBuffers(CommandBufferArray& cmdBufs, FrameResources* fr);
	// copy all cmd buffers here before calling vkQueueSubmit
	CommandBufferArray submitCommandBuffers;
    // chain device feature structures, pNext pointer has to be directly after type
    void chainNextDeviceFeature(void* elder, void* child);
    bool isExtensionAvailable(const std::vector<const char*>& availableExtensions, const char* extensionName) {
        return std::find_if(availableExtensions.begin(), availableExtensions.end(),
            [extensionName](const std::string& ext) {
                return strcmp(ext.c_str(), extensionName) == 0;
            }) != availableExtensions.end();
    }

    // check extension support and optionally return list of supported extensions
	void getInstanceExtensionSupport(std::vector<std::string>* list) {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		if (list != nullptr) {
			for (const auto& extension : extensions) {
				list->push_back(extension.extensionName);
			}
		}
	}

	struct DeviceInfo {
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceProperties2 properties2;
		VkPhysicalDeviceFeatures features;
        std::vector<std::string> extensions;
        bool suitable = false;
	};

	bool checkFeatureMeshShader(DeviceInfo& info);
    bool checkFeatureCompressedTextures(DeviceInfo& info);
    bool checkFeatureDescriptorIndexSize(DeviceInfo& info);
    // swap chain support check, only available after real device creation, not during device selection
    bool checkFeatureSwapChain(VkPhysicalDevice physDevice);
	// check extension support and optionally return list of supported extensions
	void getDeviceExtensionSupport(VkPhysicalDevice device, std::vector<std::string>* list) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
		if (list != nullptr) {
			for (const auto& extension : extensions) {
				list->push_back(extension.extensionName);
			}
		}
	}

	std::vector<DeviceInfo> deviceInfos;
	void gatherDeviceInfos();
    void pickDevice();
	bool isDeviceSuitable(DeviceInfo& info);

	// init vulkan instance, using no extensions is needed for gathering device info at startup
	void initVulkanInstance();

	// list queue flags as text
	std::string getQueueFlagsString(VkQueueFlags flags);

	// swap chain query
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

};

