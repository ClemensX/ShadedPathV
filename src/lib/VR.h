#pragma once
#if defined(OPENXR_AVAILABLE)
// OpenXR VR implementation, see https://github.com/KhronosGroup/OpenXR-SDK-Source.git
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
	void frameBegin(ThreadResources& tr);
	void frameEnd(ThreadResources& tr);
	DebugOutput debugOutput;  // This redirects std::cerr and std::cout to the IDE's output or Android Studio's logcat.
private:
	ShadedPathEngine& engine;
	XrInstance instance = nullptr;
	XrSystemId systemId;
	XrSystemProperties xrProp{};
	XrSession session = nullptr;
	XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
	bool sessionRunning = false;
	bool applicationRunning = false;
	//XrSpace sceneSpace = nullptr;
	XrSpace localSpace = nullptr;
	std::vector<XrViewConfigurationView> xrConfigViews;
	std::vector<XrViewConfigurationType> m_applicationViewConfigurations = { XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO };
	std::vector<XrViewConfigurationType> m_viewConfigurations;
	XrViewConfigurationType m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
	std::vector<XrViewConfigurationView> m_viewConfigurationViews;

	struct SwapchainInfo {
		XrSwapchain swapchain = XR_NULL_HANDLE;
		int64_t swapchainFormat = 0;
		std::vector<void*> imageViews;
	};
	std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
	std::vector<SwapchainInfo> m_depthSwapchainInfos = {};
	// init calls
	void createSystem();
	void createSession();
	void endSession();
	void createSpace();
	void GetViewConfigurationViews();
	void CreateSwapchains();
	int64_t selectColorSwapchainFormat(std::vector<int64_t> formats);
	int64_t selectDepthSwapchainFormat(std::vector<int64_t> formats);
	void DestroySwapchains();
	struct RenderLayerInfo {
		XrTime predictedDisplayTime;
		std::vector<XrCompositionLayerBaseHeader*> layers;
		XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
	};
	bool RenderLayer(RenderLayerInfo& layerInfo);

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
	void frameBegin(ThreadResources& tr){};
	void frameEnd(ThreadResources& tr){};
	private:
		ShadedPathEngine& engine;
};
#endif
