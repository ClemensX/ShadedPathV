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
	// create XR Session
	void createSession();

	// if false we run without VR
	bool enabled = false;

	// extensions: XR_KHR_vulkan_enable2, XR_EXT_hand_tracking
	const std::vector<std::string> REQUIRED_XR_EXTENSIONS { "XR_KHR_vulkan_enable2", "XR_EXT_hand_tracking" };

	// Transferred from Sample Code:
	void logLayersAndExtensions();
	void createInstanceInternal();
	inline std::string GetXrVersionString(XrVersion ver) {
		return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
	}

	// threaded frame generation
	void frameBegin(ThreadResources& tr);
	void frameEnd(ThreadResources& tr);
private:
	ShadedPathEngine& engine;
	XrInstance instance = nullptr;
	XrSystemId systemId;
	XrSystemProperties xrProp{};
	XrSession session = nullptr;
	XrSpace sceneSpace = nullptr;
	std::vector<XrViewConfigurationView> xrConfigViews;

	// init calls
	void createSystem();
	void endSession();
	void createSpace();

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
