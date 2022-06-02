#include "pch.h"


#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "openxr_loaderd")
#else
#pragma comment(lib, "openxr_loader")
#endif
#endif

void VR::init()
{
    if (!engine.isVR()) return;
	uint32_t instanceExtensionCount;
	const char* layerName = nullptr;
	auto xrResult = xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr);
	if (XR_FAILED(xrResult)) {
		enabled = false;
		Log("OpenXR intialization failed - running without VR, reverting to Stereo mode" << endl);
        engine.enableVR(false);
		return;
	}
	enabled = true;
	//CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
    if (engine.global.LIST_EXTENSIONS) {
        logLayersAndExtensions();
    }
    if (enabled) {
        createInstanceInternal();
        createSystem();
        //createSession();
    }
}

void VR::initVulkanEnable2(VkInstanceCreateInfo &instInfo)
{
    // xrCreateSession: failed to call xr*GetGraphicsRequirements before xrCreateSession
    XrGraphicsRequirementsVulkan2KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirements2KHR)));
    CHECK_XRCMD(pfnGetVulkanGraphicsRequirements2KHR(instance, systemId, &graphicsRequirements));

    // init vulkan instance
    VkResult err;
    PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanInstanceKHR)));
    XrVulkanInstanceCreateInfoKHR createInfo{ XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
    createInfo.systemId = systemId;
    createInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    createInfo.vulkanCreateInfo = &instInfo;
    createInfo.vulkanAllocator = nullptr;
    CHECK_XRCMD(pfnCreateVulkanInstanceKHR(instance, &createInfo, &engine.global.vkInstance, &err));
    if (err != VK_SUCCESS) {
        Error("Could not initialize vulkan instance via OpenXR");
    }

    // pick physical device
    XrVulkanGraphicsDeviceGetInfoKHR deviceInfo{ XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
    deviceInfo.systemId = systemId;
    deviceInfo.vulkanInstance = engine.global.vkInstance;
    PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2KHR = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsDevice2KHR)));
    pfnGetVulkanGraphicsDevice2KHR(instance, &deviceInfo, &engine.global.physicalDevice);

}

void VR::initVulkanCreateDevice(VkDeviceCreateInfo& vkCreateInfo)
{
    VkResult err;
    PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanDeviceKHR)));
    XrVulkanDeviceCreateInfoKHR createInfo{ XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
    createInfo.systemId = systemId;
    createInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    createInfo.vulkanPhysicalDevice = engine.global.physicalDevice;
    createInfo.vulkanCreateInfo = &vkCreateInfo;
    createInfo.vulkanAllocator = nullptr;
    CHECK_XRCMD(pfnCreateVulkanDeviceKHR(instance, &createInfo, &engine.global.device, &err));
    if (err != VK_SUCCESS) {
        Error("Could not initialize vulkan device via OpenXR");
    }
}

void VR::logLayersAndExtensions() {
    if (!engine.isVR()) return;
    // Write out extension properties for a given layer.
    const auto logExtensions = [this](const char* layerName, int indent = 0) {
        uint32_t instanceExtensionCount;
        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));

        std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
        for (XrExtensionProperties& extension : extensions) {
            extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        }

        CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(), &instanceExtensionCount,
            extensions.data()));

        const std::string indentStr(indent, ' ');
        LogX(Fmt("%sAvailable OpenXR Extensions: (%d)", indentStr.c_str(), instanceExtensionCount));
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

void VR::createInstanceInternal() {
    if (!engine.isVR()) return;
    CHECK(instance == XR_NULL_HANDLE);

    // Create union of extensions required by platform and graphics plugins.
    std::vector<const char*> extensions;

    // Transform our needed extensions from std::strings to C strings.
    std::transform(REQUIRED_XR_EXTENSIONS.begin(), REQUIRED_XR_EXTENSIONS.end(), std::back_inserter(extensions),
        [](const std::string& ext) { return ext.c_str(); });
    //XrApplicationInfo appInfo{};
    //appInfo.applicationName = engine.appname.c_str();

    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    createInfo.next = nullptr;
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.enabledExtensionNames = extensions.data();

    strcpy_s(createInfo.applicationInfo.applicationName, engine.appname.c_str());
    strcpy_s(createInfo.applicationInfo.engineName, engine.engineName.c_str());
    createInfo.applicationInfo.engineVersion = engine.engineVersionInt;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    try {
        CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));
    }
    catch (exception e) {
        enabled = false;
        Log("OpenXR instance creation failed - running without VR" << endl);
        return;
    }
    Log("OpenXR instance created successfully!" << endl);
}

void VR::createSystem()
{
    XrSystemGetInfo sysGetInfo{ .type = XR_TYPE_SYSTEM_GET_INFO };
    sysGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    //sysGetInfo.formFactor = XR_FORM_FACTOR_HANDHELD_DISPLAY; // force XR error to see if error system is working
    CHECK_XRCMD(xrGetSystem(instance, &sysGetInfo, &systemId));
    CHECK_XRCMD(xrGetSystemProperties(instance, systemId, &xrProp));
    Log("VR running on " << xrProp.systemName << " vendor id " << xrProp.vendorId << endl);
    
    // max swapchain image sizes: xrProp.graphicsProperties.maxSwapchainImageHeight and ...Width
}

void VR::createSession()
{

    // xrCreateSession: failed to call xrGetVulkanGraphicsDevice before xrCreateSession
    // basic session creation:
    XrGraphicsBindingVulkanKHR binding = { XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
    binding.instance = engine.global.vkInstance;
    binding.physicalDevice = engine.global.physicalDevice;
    binding.device = engine.global.device;
    binding.queueFamilyIndex = engine.global.presentQueueFamiliyIndex;
    binding.queueIndex = engine.global.presentQueueIndex;
    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &binding;
    sessionInfo.systemId = systemId;
    CHECK_XRCMD(xrCreateSession(instance, &sessionInfo, &session));


    // Enumerate the view configurations paths.
    uint32_t configurationCount;
    CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, 0, &configurationCount, nullptr));

    std::vector<XrViewConfigurationType> configurationTypes(configurationCount);
    CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, configurationCount, &configurationCount, configurationTypes.data()));

    bool configFound = false;
    for (uint32_t i = 0; i < configurationCount; ++i)
    {
        if (configurationTypes[i] == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
        {
            configFound = true;
            break;  // Pick the first supported, i.e. preferred, view configuration.
        }
    }

    if (!configFound)
        return;   // Cannot support any view configuration of this system.

    // Get detailed information of each view element.
    uint32_t viewCount;
    CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        0,
        &viewCount,
        nullptr));

    std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        viewCount,
        &viewCount,
        configViews.data()));
    xrConfigViews = configViews;
    // Set the primary view configuration for the session.
    XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    CHECK_XRCMD(xrBeginSession(session, &beginInfo));

    // Allocate a buffer according to viewCount.
    //std::vector<XrView> views(viewCount, { XR_TYPE_VIEW });
    createSpace();
}

void VR::createSpace()
{
    // check if ground based bounds are available
    // Valve Index: At least one base must be connected and visible in SteamVR, HMD may be inactive
    XrExtent2Df bounds;
    auto res = xrGetReferenceSpaceBoundsRect(session, XR_REFERENCE_SPACE_TYPE_STAGE, &bounds);
    if (res != XR_SUCCESS) {
        THROW_XR(instance, res);
    }
    Log("XR ground bounds: " << bounds.width << " " << bounds.height << endl);
}

void VR::frameBegin(ThreadResources& tr)
{
    // Wait for a new frame.
    XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
    CHECK_XRCMD(xrWaitFrame(session, &frameWaitInfo, &tr.frameState));

    // Begin frame immediately before GPU work
    XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
    CHECK_XRCMD(xrBeginFrame(session, &frameBeginInfo));

    XrCompositionLayerProjectionView projViews[2] = { /*...*/ };
    XrCompositionLayerProjection layerProj{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

    if (tr.frameState.shouldRender) {
        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.displayTime = tr.frameState.predictedDisplayTime;
        viewLocateInfo.space = sceneSpace;

        XrViewState viewState{ XR_TYPE_VIEW_STATE };
        XrView views[2] = { {XR_TYPE_VIEW}, {XR_TYPE_VIEW} };
        uint32_t viewCountOutput;
        CHECK_XRCMD(xrLocateViews(session, &viewLocateInfo, &viewState, xrConfigViews.size(), &viewCountOutput, views));

        // ...
        // Use viewState and frameState for scene render, and fill in projViews[2]
        // ...

        // Assemble composition layers structure
        layerProj.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        layerProj.space = sceneSpace;
        layerProj.viewCount = 2;
        layerProj.views = projViews;
        tr.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProj));
    }
}

void VR::frameEnd(ThreadResources& tr)
{
    // End frame and submit layers, even if layers is empty due to shouldRender = false
    XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
    frameEndInfo.displayTime = tr.frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = (uint32_t)tr.layers.size();
    frameEndInfo.layers = tr.layers.data();
    CHECK_XRCMD(xrEndFrame(session, &frameEndInfo));
    tr.layers.clear();
}

void VR::endSession()
{
    CHECK_XRCMD(xrDestroySession(session));
}

VR::~VR()
{
    if (!engine.isVR()) return;
    endSession();
    if (instance != XR_NULL_HANDLE) {
        xrDestroyInstance(instance);
        Log("OpenXR instance destroyed" << endl);
    }

}