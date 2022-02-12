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
	enabled = true;
	//CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
    LogLayersAndExtensions();
}

void VR::LogLayersAndExtensions() {
    // Write out extension properties for a given layer.
    const auto logExtensions = [](const char* layerName, int indent = 0) {
        uint32_t instanceExtensionCount;
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));

        std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
        for (XrExtensionProperties& extension : extensions) {
            extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        }

        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(), &instanceExtensionCount,
            extensions.data()));

        const std::string indentStr(indent, ' ');
        LogX(Fmt("%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount));
        for (const XrExtensionProperties& extension : extensions) {
            LogX(Fmt("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName, extension.extensionVersion));
        }
    };

    // Log non-layer extensions (layerName==nullptr).
    logExtensions(nullptr);

    // Log layers and any of their extensions.
    {
        uint32_t layerCount;
        CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));

        std::vector<XrApiLayerProperties> layers(layerCount);
        for (XrApiLayerProperties& layer : layers) {
            layer.type = XR_TYPE_API_LAYER_PROPERTIES;
        }

        CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

        LogX(Fmt("Available Layers: (%d)", layerCount));
        for (const XrApiLayerProperties& layer : layers) {
            LogX(Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName,
                    GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description));
            logExtensions(layer.layerName, 4);
        }
    }
}


VR::~VR()
{
}