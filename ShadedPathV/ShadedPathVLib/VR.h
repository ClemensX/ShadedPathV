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

	// Transferred from Sample Code:
	void LogLayersAndExtensions();
	inline std::string GetXrVersionString(XrVersion ver) {
		return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
	}


private:
	ShadedPathEngine& engine;

};


