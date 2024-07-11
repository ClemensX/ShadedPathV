#include "mainheader.h"

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "openxr_loaderd")
#else
#pragma comment(lib, "openxr_loader")
#endif
#endif

using namespace std;

void VR::init()
{
    if (!engine.isVR()) return;
	uint32_t instanceExtensionCount;
	const char* layerName = nullptr;
    XR_TUT_LOG("OpenXR Tutorial Chapter 2");
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

    strcpy(createInfo.applicationInfo.applicationName, engine.appname.c_str());
    strcpy(createInfo.applicationInfo.engineName, engine.engineName.c_str());
    createInfo.applicationInfo.engineVersion = engine.engineVersionInt;
    createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;//XR_CURRENT_API_VERSION;

    auto xrResult = xrCreateInstance(&createInfo, &instance);
    if (XR_FAILED(xrResult)) {
        enabled = false;
        Log("Failed to create Instance. Make sure your VR runtime and headset are working properly. Running without VR" << endl);
        engine.enableVR(false);
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
    XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
    OPENXR_CHECK(xrGetInstanceProperties(instance, &instanceProperties), "Failed to get InstanceProperties.");

    XR_TUT_LOG("OpenXR Runtime: " << instanceProperties.runtimeName << " - "
        << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
        << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
        << XR_VERSION_PATCH(instanceProperties.runtimeVersion));
}

void VR::create()
{
    createSession();
    createSpace();
    GetViewConfigurationViews();
    CreateSwapchains();
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


    //// Enumerate the view configurations paths.
    //uint32_t configurationCount;
    //CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, 0, &configurationCount, nullptr));

    //std::vector<XrViewConfigurationType> configurationTypes(configurationCount);
    //CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, configurationCount, &configurationCount, configurationTypes.data()));

    //bool configFound = false;
    //for (uint32_t i = 0; i < configurationCount; ++i)
    //{
    //    if (configurationTypes[i] == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
    //    {
    //        configFound = true;
    //        break;  // Pick the first supported, i.e. preferred, view configuration.
    //    }
    //}

    //if (!configFound)
    //    return;   // Cannot support any view configuration of this system.

    //// Get detailed information of each view element.
    //uint32_t viewCount;
    //CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId,
    //    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    //    0,
    //    &viewCount,
    //    nullptr));

    //std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    //CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId,
    //    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    //    viewCount,
    //    &viewCount,
    //    configViews.data()));
    //xrConfigViews = configViews;
    // Set the primary view configuration for the session.
    //XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
    //beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    //CHECK_XRCMD(xrBeginSession(session, &beginInfo));

    //// Allocate a buffer according to viewCount.
    ////std::vector<XrView> views(viewCount, { XR_TYPE_VIEW });
}

void VR::createSpace()
{
    // check if ground based bounds are available
    // Valve Index: At least one base must be connected and visible in SteamVR, HMD may be inactive
    XrExtent2Df bounds;
    auto res = xrGetReferenceSpaceBoundsRect(session, XR_REFERENCE_SPACE_TYPE_STAGE, &bounds);
    if (res != XR_SUCCESS) {
        Log("WARNING XR ground bounds not available. Can happen if HMD is not visible." << endl);
    } else {
        Log("XR ground bounds: " << bounds.width << " " << bounds.height << endl);
    }
    XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    referenceSpaceCI.poseInReferenceSpace = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f} };
    OPENXR_CHECK(xrCreateReferenceSpace(session, &referenceSpaceCI, &localSpace), "Failed to create ReferenceSpace.");
}

void VR::GetViewConfigurationViews()
{
    // Gets the View Configuration Types. The first call gets the count of the array that will be returned. The next call fills out the array.
    uint32_t viewConfigurationCount = 0;
    OPENXR_CHECK(xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigurationCount, nullptr), "Failed to enumerate View Configurations.");
    m_viewConfigurations.resize(viewConfigurationCount);
    OPENXR_CHECK(xrEnumerateViewConfigurations(instance, systemId, viewConfigurationCount, &viewConfigurationCount, m_viewConfigurations.data()), "Failed to enumerate View Configurations.");

    // Pick the first application supported View Configuration Type con supported by the hardware.
    for (const XrViewConfigurationType& viewConfiguration : m_applicationViewConfigurations) {
        if (std::find(m_viewConfigurations.begin(), m_viewConfigurations.end(), viewConfiguration) != m_viewConfigurations.end()) {
            m_viewConfiguration = viewConfiguration;
            break;
        }
    }
    if (m_viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM) {
        std::cerr << "Failed to find a view configuration type. Defaulting to XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO." << std::endl;
        m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    }

    // Gets the View Configuration Views. The first call gets the count of the array that will be returned. The next call fills out the array.
    uint32_t viewConfigurationViewCount = 0;
    OPENXR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId, m_viewConfiguration, 0, &viewConfigurationViewCount, nullptr), "Failed to enumerate ViewConfiguration Views.");
    m_viewConfigurationViews.resize(viewConfigurationViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    OPENXR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId, m_viewConfiguration, viewConfigurationViewCount, &viewConfigurationViewCount, m_viewConfigurationViews.data()), "Failed to enumerate ViewConfiguration Views.");
}

int64_t VR::selectColorSwapchainFormat(std::vector<int64_t> formats)
{
    for (int64_t format : formats) {
        if (format == GlobalRendering::ImageFormat) {
            return format;
        }
    }
    Error("OpenXR failed to find a supported color swapchain format.");
}

int64_t VR::selectDepthSwapchainFormat(std::vector<int64_t> formats)
{
    for (int64_t format : formats) {
        if (format == GlobalRendering::depthFormat) {
            return format;
        }
    }
    Error("OpenXR failed to find a supported depth swapchain format.");
}

XrSwapchainImageBaseHeader* VR::AllocateSwapchainImageData(XrSwapchain swapchain, VR::SwapchainType type, uint32_t count) {
    swapchainImagesMap[swapchain].first = type;
    swapchainImagesMap[swapchain].second.resize(count, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
    return reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchainImagesMap[swapchain].second.data());
}

VkImage VR::GetSwapchainImage(XrSwapchain swapchain, uint32_t index) {
    VkImage image = swapchainImagesMap[swapchain].second[index].image;
    VkImageLayout layout = swapchainImagesMap[swapchain].first == VR::SwapchainType::COLOR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    imageStates[image] = layout;
    return image;
}

void VR::CreateSwapchains()
{
    // Get the supported swapchain formats as an array of int64_t and ordered by runtime preference.
    uint32_t formatCount = 0;
    OPENXR_CHECK(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr), "Failed to enumerate Swapchain Formats");
    std::vector<int64_t> formats(formatCount);
    OPENXR_CHECK(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data()), "Failed to enumerate Swapchain Formats");
    int64_t swapchainColorFormat = selectColorSwapchainFormat(formats);
    int64_t swapchainDepthFormat = selectDepthSwapchainFormat(formats);
    //Resize the SwapchainInfo to match the number of view in the View Configuration.
    m_colorSwapchainInfos.resize(m_viewConfigurationViews.size());
    m_depthSwapchainInfos.resize(m_viewConfigurationViews.size());
    for (size_t i = 0; i < m_viewConfigurationViews.size(); i++) {
        SwapchainInfo& colorSwapchainInfo = m_colorSwapchainInfos[i];
        SwapchainInfo& depthSwapchainInfo = m_depthSwapchainInfos[i];

        // Fill out an XrSwapchainCreateInfo structure and create an XrSwapchain.
        // Color.
        XrSwapchainCreateInfo swapchainCI{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapchainCI.createFlags = 0;
        swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.format = swapchainColorFormat;
        swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
        swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
        swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
        swapchainCI.faceCount = 1;
        swapchainCI.arraySize = 1;
        swapchainCI.mipCount = 1;
        OPENXR_CHECK(xrCreateSwapchain(session, &swapchainCI, &colorSwapchainInfo.swapchain), "Failed to create Color Swapchain");
        colorSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

        // Depth.
        swapchainCI.createFlags = 0;
        swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        swapchainCI.format = swapchainDepthFormat;
        swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
        swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
        swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
        swapchainCI.faceCount = 1;
        swapchainCI.arraySize = 1;
        swapchainCI.mipCount = 1;
        OPENXR_CHECK(xrCreateSwapchain(session, &swapchainCI, &depthSwapchainInfo.swapchain), "Failed to create Depth Swapchain");
        depthSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

        // Get the number of images in the color/depth swapchain and allocate Swapchain image data via GraphicsAPI to store the returned array.
        uint32_t colorSwapchainImageCount = 0;
        OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr), "Failed to enumerate Color Swapchain Images.");
        XrSwapchainImageBaseHeader* colorSwapchainImages = AllocateSwapchainImageData(colorSwapchainInfo.swapchain, VR::SwapchainType::COLOR, colorSwapchainImageCount);
        OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages), "Failed to enumerate Color Swapchain Images.");

        uint32_t depthSwapchainImageCount = 0;
        OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr), "Failed to enumerate Depth Swapchain Images.");
        XrSwapchainImageBaseHeader* depthSwapchainImages = AllocateSwapchainImageData(depthSwapchainInfo.swapchain, VR::SwapchainType::DEPTH, depthSwapchainImageCount);
        OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages), "Failed to enumerate Depth Swapchain Images.");

        // Per image in the swapchains, fill out a GraphicsAPI::ImageViewCreateInfo structure and create a color/depth image view.
        for (uint32_t j = 0; j < colorSwapchainImageCount; j++) {
            auto image = GetSwapchainImage(colorSwapchainInfo.swapchain, j);
            auto v = engine.global.createImageView(image, (VkFormat)colorSwapchainInfo.swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            colorSwapchainInfo.imageViews.push_back(v);
        }
        for (uint32_t j = 0; j < depthSwapchainImageCount; j++) {
            auto image = GetSwapchainImage(depthSwapchainInfo.swapchain, j);
            auto v = engine.global.createImageView(image, (VkFormat)depthSwapchainInfo.swapchainFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            depthSwapchainInfo.imageViews.push_back(v);
        }
    }
}

void VR::DestroySwapchains()
{
    // Per view in the view configuration:
    for (size_t i = 0; i < m_viewConfigurationViews.size(); i++) {
        SwapchainInfo& colorSwapchainInfo = m_colorSwapchainInfos[i];
        SwapchainInfo& depthSwapchainInfo = m_depthSwapchainInfos[i];

        // Destroy the color and depth image views from GraphicsAPI.
        for (VkImageView& imageView : colorSwapchainInfo.imageViews) {
            vkDestroyImageView(engine.global.device, imageView, nullptr);
        }
        for (VkImageView& imageView : depthSwapchainInfo.imageViews) {
            vkDestroyImageView(engine.global.device, imageView, nullptr);
        }

        // Free the Swapchain Image Data.
        swapchainImagesMap[colorSwapchainInfo.swapchain].second.clear();
        swapchainImagesMap[depthSwapchainInfo.swapchain].second.clear();
        swapchainImagesMap.erase(colorSwapchainInfo.swapchain);
        swapchainImagesMap.erase(depthSwapchainInfo.swapchain);

        // Destroy the swapchains.
        OPENXR_CHECK(xrDestroySwapchain(colorSwapchainInfo.swapchain), "Failed to destroy Color Swapchain");
        OPENXR_CHECK(xrDestroySwapchain(depthSwapchainInfo.swapchain), "Failed to destroy Depth Swapchain");
    }
}


void VR::pollEvent()
{
    if (!engine.isVR()) return;
    // Poll OpenXR for a new event.
    XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };
    auto XrPollEvents = [&]() -> bool {
        eventData = { XR_TYPE_EVENT_DATA_BUFFER };
        return xrPollEvent(instance, &eventData) == XR_SUCCESS;
        };

    while (XrPollEvents()) {
        switch (eventData.type) {
            // Log the number of lost events from the runtime.
        case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
            XrEventDataEventsLost* eventsLost = reinterpret_cast<XrEventDataEventsLost*>(&eventData);
            XR_TUT_LOG("OPENXR: Events Lost: " << eventsLost->lostEventCount);
            break;
        }
                                           // Log that an instance loss is pending and shutdown the application.
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
            XrEventDataInstanceLossPending* instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending*>(&eventData);
            XR_TUT_LOG("OPENXR: Instance Loss Pending at: " << instanceLossPending->lossTime);
            sessionRunning = false;
            applicationRunning = false;
            break;
        }
                                                     // Log that the interaction profile has changed.
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
            XrEventDataInteractionProfileChanged* interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged*>(&eventData);
            XR_TUT_LOG("OPENXR: Interaction Profile changed for Session: " << interactionProfileChanged->session);
            if (interactionProfileChanged->session != session) {
                XR_TUT_LOG("XrEventDataInteractionProfileChanged for unknown Session");
                break;
            }
            break;
        }
                                                           // Log that there's a reference space change pending.
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
            XrEventDataReferenceSpaceChangePending* referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending*>(&eventData);
            XR_TUT_LOG("OPENXR: Reference Space Change pending for Session: " << referenceSpaceChangePending->session);
            if (referenceSpaceChangePending->session != session) {
                XR_TUT_LOG("XrEventDataReferenceSpaceChangePending for unknown Session");
                break;
            }
            break;
        }
                                                              // Session State changes:
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            XrEventDataSessionStateChanged* sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
            if (sessionStateChanged->session != session) {
                XR_TUT_LOG("XrEventDataSessionStateChanged for unknown Session");
                break;
            }

            if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
                // SessionState is ready. Begin the XrSession using the XrViewConfigurationType.
                XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
                sessionBeginInfo.primaryViewConfigurationType = m_viewConfiguration;
                OPENXR_CHECK(xrBeginSession(session, &sessionBeginInfo), "Failed to begin Session.");
                sessionRunning = true;
            }
            if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
                // SessionState is stopping. End the XrSession.
                OPENXR_CHECK(xrEndSession(session), "Failed to end Session.");
                sessionRunning = false;
            }
            if (sessionStateChanged->state == XR_SESSION_STATE_EXITING) {
                // SessionState is exiting. Exit the application.
                sessionRunning = false;
                applicationRunning = false;
            }
            if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING) {
                // SessionState is loss pending. Exit the application.
                // It's possible to try a reestablish an XrInstance and XrSession, but we will simply exit here.
                sessionRunning = false;
                applicationRunning = false;
            }
            // Store state for reference across the application.
            sessionState = sessionStateChanged->state;
            break;
        }
        default: {
            break;
        }
        }
    }
}

void VR::frameBegin(ThreadResources& tr)
{
    // Wait for a new frame.
    XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
    CHECK_XRCMD(xrWaitFrame(session, &frameWaitInfo, &tr.frameState));

    // Begin frame immediately before GPU work
    XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
    CHECK_XRCMD(xrBeginFrame(session, &frameBeginInfo));

    // Variables for rendering and layer composition.
    bool rendered = false;
    RenderLayerInfo renderLayerInfo;
    renderLayerInfo.predictedDisplayTime = tr.frameState.predictedDisplayTime;

    // Check that the session is active and that we should render.
    bool sessionActive = (sessionState == XR_SESSION_STATE_SYNCHRONIZED || sessionState == XR_SESSION_STATE_VISIBLE || sessionState == XR_SESSION_STATE_FOCUSED);
    if (sessionActive && tr.frameState.shouldRender) {
        // Render the stereo image and associate one of swapchain images with the XrCompositionLayerProjection structure.
        rendered = RenderLayer(renderLayerInfo);
        if (rendered) {
            renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&renderLayerInfo.layerProjection));
        }
    }

    // Tell OpenXR that we are finished with this frame; specifying its display time, environment blending and layers.
    XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
    frameEndInfo.displayTime = tr.frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
    frameEndInfo.layers = renderLayerInfo.layers.data();
    OPENXR_CHECK(xrEndFrame(session, &frameEndInfo), "Failed to end the XR Frame.");

    //XrCompositionLayerProjectionView projViews[2] = { /*...*/ };
    //XrCompositionLayerProjection layerProj{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

    //if (tr.frameState.shouldRender) {
    //    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    //    viewLocateInfo.displayTime = tr.frameState.predictedDisplayTime;
    //    viewLocateInfo.space = localSpace;
    //    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    //    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    //    XrView views[2] = { {XR_TYPE_VIEW}, {XR_TYPE_VIEW} };
    //    uint32_t viewCountOutput;
    //    CHECK_XRCMD(xrLocateViews(session, &viewLocateInfo, &viewState, static_cast<uint32_t>(xrConfigViews.size()), &viewCountOutput, views));

    //    // ...
    //    // Use viewState and frameState for scene render, and fill in projViews[2]
    //    // ...

    //    // Assemble composition layers structure
    //    layerProj.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    //    layerProj.space = localSpace;
    //    layerProj.viewCount = 2;
    //    layerProj.views = projViews;
    //    tr.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProj));
    //}
}

void VR::frameEnd(ThreadResources& tr)
{
    //// End frame and submit layers, even if layers is empty due to shouldRender = false
    //XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
    //frameEndInfo.displayTime = tr.frameState.predictedDisplayTime;
    //frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    //frameEndInfo.layerCount = (uint32_t)tr.layers.size();
    //frameEndInfo.layers = tr.layers.data();
    //CHECK_XRCMD(xrEndFrame(session, &frameEndInfo));
    //tr.layers.clear();
}

bool VR::RenderLayer(RenderLayerInfo& layerInfo)
{
    return false;
}

void VR::endSession()
{
    DestroySwapchains();
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