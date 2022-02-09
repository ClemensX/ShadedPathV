#include "pch.h"

void VR::init()
{
	uint32_t instanceExtensionCount;
	const char* layerName = nullptr;
	auto xrResult = xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr);
	if (XR_FAILED(xrResult)) {
		enabled = false;
		Log("OpenXR intialization failed - running without VR" << endl);
		return;
	}
	//CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
}

VR::~VR()
{
}