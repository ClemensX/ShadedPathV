#pragma once

// OpenXR VR implementation, see https://github.com/KhronosGroup/OpenXR-SDK-Source.git
class VR
{
public:
	VR(ShadedPathEngine& s) : engine(s) {
		Log("VR c'tor\n");
	};
	~VR();

	void init();
	// if false we run without VR
	bool enabled = false;
	void initAfterDeviceCreation();
	void initGLFW();
	void createPresentQueue(unsigned int value);

	bool shouldClose();

	// extensions: XR_KHR_vulkan_enable2, XR_EXT_hand_tracking
	const vector<string> REQUIRED_XR_EXTENSIONS { "XR_KHR_vulkan_enable2", "XR_EXT_hand_tracking" };

	// Transferred from Sample Code:
	void logLayersAndExtensions();
	void createInstanceInternal();
	inline std::string GetXrVersionString(XrVersion ver) {
		return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
	}

private:
	ShadedPathEngine& engine;
	XrInstance instance = nullptr;
	XrSystemId systemId;
	XrSystemProperties xrProp{};
	XrSession session = nullptr;
	XrSpace sceneSpace = nullptr;
	vector<XrViewConfigurationView> xrConfigViews;

	// init calls
	void createSystem();
	void createSession();
	void endSession();

	// threaded frame generation
	void frameBegin(ThreadResources &tr);
	void frameEnd(ThreadResources& tr);
};


