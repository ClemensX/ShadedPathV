#pragma once
#if defined(OPENXR_AVAILABLE)
// OpenXR VR implementation, see https://github.com/KhronosGroup/OpenXR-SDK-Source.git
enum GraphicsAPI_Type : uint8_t {
	UNKNOWN,
	D3D11,
	D3D12,
	OPENGL,
	OPENGL_ES,
	VULKAN
};
// include xr linear algebra for XrVector and XrMatrix classes.
#include "xr_linear_algebra.h"
class VR
{
public:
	VR(ShadedPathEngine& s) : engine(s) {
		Log("VR c'tor\n");
	};
	~VR();

	// VR has to be initialized before Vulkan instance creation
	void init();

	// initialize Vulkan instance via XR_KHR_vulkan_enable2 extension
	void initVulkanEnable2(VkInstanceCreateInfo &instInfo);
	// create logical devive via XR_KHR_vulkan_enable2 extension
	void initVulkanCreateDevice(VkDeviceCreateInfo& createInfo);
	// create XR stuff
	void create();

	// if false we run without VR
	bool enabled = false;

	// extensions: XR_KHR_vulkan_enable2, XR_EXT_hand_tracking
#   if defined(_DEBUG)
	const std::vector<std::string> REQUIRED_XR_EXTENSIONS { "XR_KHR_vulkan_enable2", "XR_EXT_hand_tracking", "XR_EXT_debug_utils"};
#   else
	const std::vector<std::string> REQUIRED_XR_EXTENSIONS{ "XR_KHR_vulkan_enable2", "XR_EXT_hand_tracking" };
#   endif

	// Transferred from Sample Code:
	void logLayersAndExtensions();
	void createInstanceInternal();
	inline std::string GetXrVersionString(XrVersion ver) {
		return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
	}

	// threaded frame generation
	void pollEvent();
    void frameWait();
	void frameBegin(ThreadResources& tr);
	void frameCopy(ThreadResources& tr);
	void frameEnd(ThreadResources& tr);
	DebugOutput debugOutput;  // This redirects std::cerr and std::cout to the IDE's output or Android Studio's logcat.
	enum class SwapchainType : uint8_t {
		COLOR,
		DEPTH
	};

	// Getter for positioner
	CameraPositioner_HMD* GetPositioner() const {
		return positioner;
	}

	// Setter for positioner
	void SetPositioner(CameraPositioner_HMD* newPositioner) {
		positioner = newPositioner;
	}
private:
	ShadedPathEngine& engine;
	XrInstance instance = nullptr;
	XrSystemId systemId;
	XrSystemProperties xrProp{};
	XrSession session = nullptr;
	XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
	bool sessionRunning = false;
	bool applicationRunning = false;
	bool THREAD_LOG = false; // log thread and frame info
	//XrSpace sceneSpace = nullptr;
	std::vector<XrViewConfigurationView> xrConfigViews;
	std::vector<XrViewConfigurationType> m_applicationViewConfigurations = { XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO };
	std::vector<XrViewConfigurationType> m_viewConfigurations;
	XrViewConfigurationType m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
	std::vector<XrViewConfigurationView> m_viewConfigurationViews;

	struct SwapchainInfo {
		XrSwapchain swapchain = XR_NULL_HANDLE;
		int64_t swapchainFormat = 0;
		std::vector<VkImageView> imageViews;
	};
	std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
	std::vector<SwapchainInfo> m_depthSwapchainInfos = {};
	std::unordered_map<XrSwapchain, std::pair<SwapchainType, std::vector<XrSwapchainImageVulkanKHR>>> swapchainImagesMap{};
	std::unordered_map<VkImage, VkImageLayout> imageStates;
	std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = { XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE };
	std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
	XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

	XrSpace m_localSpace = XR_NULL_HANDLE;
	struct RenderLayerInfo {
		XrTime predictedDisplayTime;
		std::vector<XrCompositionLayerBaseHeader*> layers;
		XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
		ThreadResources* tr = nullptr;
		uint32_t viewCount = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t colorImageIndex = 0;
		bool renderStarted = false;	
		//XrSwapchain colorSwapchain = XR_NULL_HANDLE;
	};
	bool RenderLayerPrepare(RenderLayerInfo& layerInfo);
	bool RenderLayerCopyRenderedImage(RenderLayerInfo& layerInfo);
	// init calls
	void createSystem();
	void GetEnvironmentBlendModes();
	void CreateReferenceSpace();
	void createSession();
	void endSession();
	void GetViewConfigurationViews();
	void CreateSwapchains();
	int64_t selectColorSwapchainFormat(std::vector<int64_t> formats);
	int64_t selectDepthSwapchainFormat(std::vector<int64_t> formats);
	XrSwapchainImageBaseHeader* AllocateSwapchainImageData(XrSwapchain swapchain, VR::SwapchainType type, uint32_t count);
	VkImage GetSwapchainImage(XrSwapchain swapchain, uint32_t index);
	void DestroySwapchains();
	void DestroyReferenceSpace();
	void DestroySession();
	void DestroyDebugMessenger();
	void DestroyInstance();
	void ClearColor(VkCommandBuffer cmdBuffer, VkImageView imageView, float r, float g, float b, float a);
	struct ImageViewCreateInfo {
		void* image;
		enum class Type : uint8_t {
			RTV,
			DSV,
			SRV,
			UAV
		} type;
		enum class View : uint8_t {
			TYPE_1D,
			TYPE_2D,
			TYPE_3D,
			TYPE_CUBE,
			TYPE_1D_ARRAY,
			TYPE_2D_ARRAY,
			TYPE_CUBE_ARRAY,
		} view;
		int64_t format;
		enum class Aspect : uint8_t {
			COLOR_BIT = 0x01,
			DEPTH_BIT = 0x02,
			STENCIL_BIT = 0x04
		} aspect;
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};
	std::unordered_map<VkImageView, ImageViewCreateInfo> imageViewResources;
	VkImageView CreateImageView(const ImageViewCreateInfo& imageViewCI);
	CameraPositioner_HMD* positioner = nullptr;
	XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	RenderLayerInfo renderLayerInfo;
};
#else
class VR
{
	public:
		VR(ShadedPathEngine& s) : engine(s) {
			Log("VR c'tor\n");
		};
		~VR(){};
	// VR has to be initialized before Vulkan instance creation
	void init() {};
	// initialize Vulkan instance via XR_KHR_vulkan_enable2 extension
	void initVulkanEnable2(VkInstanceCreateInfo &instInfo){};
	// create logical devive via XR_KHR_vulkan_enable2 extension
	void initVulkanCreateDevice(VkDeviceCreateInfo& createInfo){};
	// create XR Session
	void createSession(){};
	// Transferred from Sample Code:
	void logLayersAndExtensions(){};
	void createInstanceInternal(){};

	// threaded frame generation
	void pollEvent() {};
	void frameWait() {};
	void frameBegin(ThreadResources& tr) {};
	void frameCopy(ThreadResources& tr) {};
	void frameEnd(ThreadResources& tr){};
	// Getter for positioner
	CameraPositioner_HMD* GetPositioner() const {
		return positioner;
	}

	// Setter for positioner
	void SetPositioner(CameraPositioner_HMD* newPositioner) {
		positioner = newPositioner;
	}
	void create() {};
private:
		ShadedPathEngine& engine;
		CameraPositioner_HMD* positioner = nullptr;
};
#endif
