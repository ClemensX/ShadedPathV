#include "mainheader.h"

using namespace std;

// some const definitions for validation and extension levels
const vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

string GlobalRendering::getVulkanAPIString()
{
    uint32_t apiNumber;
    VkResult res = vkEnumerateInstanceVersion(&apiNumber);
    if (res != VK_SUCCESS) Error("Cannot call vulkan API");
    uint32_t major = VK_API_VERSION_MAJOR(apiNumber);
    uint32_t minor = VK_API_VERSION_MINOR(apiNumber);
    uint32_t patch = VK_API_VERSION_PATCH(apiNumber);
    uint32_t variant = VK_API_VERSION_VARIANT(apiNumber);
    stringstream vulkan_version;
    vulkan_version << "" << major << "." << minor << "." << patch;
    return vulkan_version.str();
}

void GlobalRendering::gatherDeviceInfos()
{
    //Log("gathering physical device capabilities.\n");
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        Error("no physical vulkan device available");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());
    Log("Found " << deviceCount << " Vulkan - supported devices : " << std::endl);
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceProperties2 properties2{};
        properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);
        vkGetPhysicalDeviceProperties2(device, &properties2);

        DeviceInfo info;
        info.device = device;
        info.properties = properties;
        info.properties2 = properties2;
        info.features = features;
        Log("  " << properties.deviceName << " API version: " << Util::decodeVulkanVersion(properties.apiVersion).c_str() << " driver version: " << Util::decodeVulkanVersion(properties.driverVersion).c_str() << " type: " << Util::decodeDeviceType(properties.deviceType) << endl);
        // get available device extensions:
        getDeviceExtensionSupport(device, &info.extensions);
        if (LIST_EXTENSIONS) {
            Log("  available device extensions:\n");
            for (const auto& extension : info.extensions) {
                Log("  " << extension << endl);
            }
        }
        info.suitable = isDeviceSuitable(info);
        deviceInfos.push_back(info);
    }
}

bool GlobalRendering::checkFeatureMeshShader(DeviceInfo& info)
{
    // check mesh support:
    // set extension details for mesh shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures = {};
    meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshFeatures.pNext = nullptr;
    VkPhysicalDeviceFeatures2 feature2{};
    feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feature2.pNext = &meshFeatures;
    vkGetPhysicalDeviceFeatures2(info.device, &feature2);
    VkPhysicalDeviceMeshShaderFeaturesEXT* meshSupport = (VkPhysicalDeviceMeshShaderFeaturesEXT*)feature2.pNext;
    //Log(meshSupport << endl);
    if (!meshSupport->meshShader || !meshSupport->taskShader) {
        Log("device does not support Mesh Shaders" << endl);
        Log("You might try not to enable Mesh Shader in app code by removing: engine->enableMeshShader()" << endl);
        return false;
    }
    return true;
}

bool GlobalRendering::checkFeatureCompressedTextures(DeviceInfo& info)
{
    // check compressed texture support:
    VkFormatProperties fp{};
    vkGetPhysicalDeviceFormatProperties(info.device, VK_FORMAT_BC7_SRGB_BLOCK, &fp);
    // 0x01d401
    VkFormatFeatureFlags flagsToCheck = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((fp.linearTilingFeatures & flagsToCheck) == 0) {
        Log("device does not support needed linearTilingFeatures" << endl);
        return false;
    }
    if ((fp.optimalTilingFeatures & flagsToCheck) == 0) {
        Log("device does not support needed optimalTilingFeatures" << endl);
        return false;
    }
    return true;
}

bool GlobalRendering::checkFeatureDescriptorIndexSize(DeviceInfo& info)
{
    // check descriptor index size suitable for texture count:
    if (info.properties.limits.maxDescriptorSetSampledImages < TextureStore::UPPER_LIMIT_TEXTURE_COUNT) {
        Log("Device does not support enough texture slots in descriptors" << endl);
        return false;
    }
    return true;
}


bool GlobalRendering::checkFeatureSwapChain(VkPhysicalDevice physDevice)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDevice);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return false;
    return true;
}

bool GlobalRendering::isDeviceSuitable(DeviceInfo& info)
{
    // check device extensions support:
    for (const auto& extensionName : deviceExtensions) {
        if (std::find(info.extensions.begin(), info.extensions.end(), extensionName) == info.extensions.end()) {
            return false;
        }
    }

    // check device features support:
    if (isExtensionAvailable(deviceExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME)) {
        if (!checkFeatureMeshShader(info)) return false;
    }
    if (!checkFeatureCompressedTextures(info)) return false;
    if (!checkFeatureDescriptorIndexSize(info)) return false;
    //if (!checkFeatureSwapChain(info)) return false;
    familyIndices = findQueueFamilies(info.device);
    if (!familyIndices.isComplete(true)) return false;

    return true;
}

void GlobalRendering::pickDevice()
{
    int fixedDeviceIndex = engine->getFixedPhysicalDeviceIndex();
    if (fixedDeviceIndex >= 0) {
        if (fixedDeviceIndex < deviceInfos.size()) {
            physicalDevice = deviceInfos[fixedDeviceIndex].device;
            physicalDeviceProperties = deviceInfos[fixedDeviceIndex].properties2;
            Log("WARNING: Using fixed device without checking feature support: " << deviceInfos[fixedDeviceIndex].properties.deviceName << endl);
        } else {
            Error("Fixed device index out of range");
        }
        return;
    }
    for (auto& info : deviceInfos) {
        if (info.suitable) {
            physicalDevice = info.device;
            physicalDeviceProperties = info.properties2;
            Log("Picked device: " << info.properties.deviceName << endl);
            return;
        }
    }
    Error("No suitable physical device found. You might consider using setFixedPhysicalDeviceIndex(0) to pre-select a device without feature support checking.");
}

void GlobalRendering::init()
{
    if (LIST_EXTENSIONS) {
        vector<string> instanceExtensionlist;
        getInstanceExtensionSupport(&instanceExtensionlist);
        Log("available Vulkan instance extensions:\n");
        for (const auto& extension : instanceExtensionlist) {
            Log("  " << extension << endl);
        }
    }
    initVulkanInstance();
    gatherDeviceInfos();
    // list needed extensions:
    Log("requested Vulkan instance extensions:" << endl)
        Util::printCStringList(instanceExtensions);
    Log("requested Vulkan device extensions:" << endl)
        Util::printCStringList(deviceExtensions);
    Log("suitable devices:\n");
    for (auto& info : deviceInfos) {
        if (info.suitable) {
            Log("  " << info.properties.deviceName << endl);
        }
    }
    pickDevice();
    //checkFeatureSwapChain(physicalDevice);
    // list queue properties:
    findQueueFamilies(physicalDevice, true);
    createLogicalDevice();
    createCommandPools();
    createTextureSampler();
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // binary is default

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &singleTimeCommandsSemaphore) != VK_SUCCESS) {
        Error("failed to create singleTimeCommandsSemaphore");
    }
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

    if (vkCreateFence(device, &fenceInfo, nullptr, &queueSubmitFence) != VK_SUCCESS) {
        Error("failed to create inFlightFence for a frame");
    }
    engine->util.debugNameObjectFence(queueSubmitFence, "GlobalRendering.queueSubmitFence");
}

void GlobalRendering::shutdown()
{
    if (queueSubmitFence != nullptr) {
        vkDestroyFence(device, queueSubmitFence, nullptr);
    }
    for (auto& sam : textureSampler) {
        if (sam != nullptr) {
            vkDestroySampler(device, sam, nullptr);
        }
    }
    vkDestroySemaphore(device, singleTimeCommandsSemaphore, nullptr);
    for (int i = 0; i < engine->numWorkerThreads; i++) {
        auto& res = workerThreadResources[i];
        vkDestroyCommandPool(device, res.commandPool, nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, commandPoolTransfer, nullptr);
    vkDestroyDevice(device, nullptr);
    device = nullptr;
    vkDestroyInstance(vkInstance, nullptr);
    vkInstance = nullptr;
    glfwTerminate();
}

void GlobalRendering::initVulkanInstance()
{
    if (vkInstance != nullptr) {
        Error("Trying to re-init vulkan instance");
    }
    vector<string> instanceExtensionlist;
    getInstanceExtensionSupport(&instanceExtensionlist);
    for (const auto& extensionName : instanceExtensions) {
        if (std::find(instanceExtensionlist.begin(), instanceExtensionlist.end(), extensionName) == instanceExtensionlist.end()) {
            Error("instance extension not supported: " + string(extensionName));
        }
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ShadedPathV";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ShadedPathV";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;//VP_KHR_ROADMAP_2022_MIN_API_VERSION;//API_VERSION;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
#   if defined(__APPLE__)    
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#   endif
    vkInstance = VK_NULL_HANDLE;
    if (engine->isVR()) {
        //engine->vr.initVulkanEnable2(createInfo);
        Error("OpenXR not supported yet");
    }
    else {
        if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
            Error("failed to create instance!");
        }
    }
}


QueueFamilyIndices GlobalRendering::findQueueFamilies(VkPhysicalDevice device, bool listmode)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            if (listmode) {
                Log("found graphics queue at index " << i << ", max queues: " << queueFamily.queueCount << endl);
            }
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
            if (listmode) {
                Log("found transfer queue at index " << i << ", max queues: " << queueFamily.queueCount << endl);
                //Log("  other queue flags: " << getQueueFlagsString(queueFamily.queueFlags).c_str() << endl);
            }
        }
        if (engine->presentationMode) {
            VkBool32 presentSupport = true; // TODO hack
            //vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }
        if (!listmode && indices.isComplete(engine->presentationMode)) {
            break;
        }
        i++;
    }
    i = 0;
    // try to find transfer only queue (supposedly better DMA performance)
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            auto flags = queueFamily.queueFlags;
            // cancel out irrelevant bits:
            flags &= ~VK_QUEUE_SPARSE_BINDING_BIT;
            flags &= ~VK_QUEUE_OPTICAL_FLOW_BIT_NV;
            flags &= ~VK_QUEUE_PROTECTED_BIT;
#           if defined(ALLOW_USING_NON_TRANSFER_ONLY_QUEUE)
            if (flags & VK_QUEUE_TRANSFER_BIT && i != indices.graphicsFamily && i != indices.presentFamily) {
                indices.transferFamily = i;
                if (listmode) {
                    Log("ALLOW_USING_NON_TRANSFER_ONLY_QUEUE found transfer queue at index " << i << ", max queues: " << queueFamily.queueCount << endl);
                    //Log("  other queue flags: " << getQueueFlagsString(queueFamily.queueFlags).c_str() << endl);
                }
                break;
            }
#           endif
            if (flags == VK_QUEUE_TRANSFER_BIT) {
                indices.transferFamily = i;
                if (listmode) {
                    Log("found transfer ONLY queue at index " << i << ", max queues: " << queueFamily.queueCount << endl);
                    //Log("  other queue flags: " << getQueueFlagsString(queueFamily.queueFlags).c_str() << endl);
                }
                break;
            }
        }
        i++;
    }
    if (indices.graphicsFamily == indices.transferFamily) {
#       if !defined(ALLOW_USING_NON_TRANSFER_ONLY_QUEUE)
            Error("ERROR: did not find fransfer only queue");
#       else
            Log("WARNING: did not find fransfer only queue" << endl);
#       endif
    }
    return indices;
}

string GlobalRendering::getQueueFlagsString(VkQueueFlags flags)
{
    string t;
    if (flags & VK_QUEUE_GRAPHICS_BIT) t += "GRAPHICS ";
    if (flags & VK_QUEUE_COMPUTE_BIT) t += "COMPUTE ";
    if (flags & VK_QUEUE_TRANSFER_BIT) t += "TRANSFER ";
    if (flags & VK_QUEUE_SPARSE_BINDING_BIT) t += "SPARSE_BINDING ";
    if (flags & VK_QUEUE_PROTECTED_BIT) t += "PROTECTED ";
    if (flags & 0x00000020) t += "VIDEO_DECODE "; // VK_QUEUE_VIDEO_DECODE_BIT_KHR
    if (flags & 0x00000040) t += "VIDEO_ENCODE "; // VK_QUEUE_VIDEO_ENCODE_BIT_KHR 
    if (flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) t += "OPTICAL_FLOW ";
    return t;
}

uint32_t GlobalRendering::findMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    return 0;
}


void GlobalRendering::chainNextDeviceFeature(void* elder, void* child)
{
    // cast so we can use pNext pointer:
    VkBaseOutStructure* pElder = (VkBaseOutStructure*)elder;
    VkBaseOutStructure* pChild = (VkBaseOutStructure*)child;
    while (pElder->pNext != nullptr) {
        pElder = pElder->pNext;
    }
    pElder->pNext = pChild;
}

void GlobalRendering::createLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { familyIndices.graphicsFamily.value() };
    uniqueQueueFamilies.insert({ familyIndices.transferFamily.value() });
    if (engine->presentationMode) {
        uniqueQueueFamilies.insert({ familyIndices.presentFamily.value() });
    }

    float queuePriority[] = {1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f};
    //float queuePriority[] {1.0f};
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority[0];
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        // provoke validation layer warning by commenting out following line:
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceFeatures.html
        .geometryShader = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .textureCompressionBC = VK_TRUE,
        .vertexPipelineStoresAndAtomics = VK_TRUE,
        .fragmentStoresAndAtomics = VK_TRUE,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
        .shaderInt64 = VK_TRUE,
        //deviceFeatures.textureCompressionETC2 = VK_TRUE; not supported on Quadro P2000 with Max-Q Design 1.3.194
        //deviceFeatures.textureCompressionASTC_LDR = VK_TRUE; not supported on Quadro P2000 with Max-Q Design 1.3.194
        //deviceFeatures.dynamicRendering
    };

    // set extension details for mesh shader
    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures = {};
    meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshFeatures.meshShader = VK_TRUE;
    meshFeatures.taskShader = VK_FALSE;

    VkPhysicalDevicePortabilitySubsetFeaturesKHR portability{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR,
        .events = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features deviceFeatures12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
        .vulkanMemoryModel = VK_TRUE,
        .vulkanMemoryModelDeviceScope = VK_TRUE,
#       if defined(__APPLE__)
        .pNext = (void*)&portability,
#       endif
    };

    VkPhysicalDeviceVulkan13Features deviceFeatures13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 deviceFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = deviceFeatures,
    };

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features{};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Features.synchronization2 = VK_TRUE;

    // disable geom shaders for mac as they don't support them TODO remove geom shaders alltogether
    // and use compute shaders instead
#   if defined(__APPLE__)
    deviceFeatures2.features.geometryShader = false;
    // use this  in shell: export MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=1
    //setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
    //MVKConfiguration conf;
    //size_t sz;
    //if (vkGetMoltenVKConfigurationMVK(VK_NULL_HANDLE, &conf, &sz) != VK_SUCCESS) Error("cannot call MoltenVK configuration");
    //Log("MoltenVK useMetalArgumentBuffers: " << conf.useMetalArgumentBuffers << endl);
    //vkSetMoltenVKConfigurationMVK();
#   endif
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    chainNextDeviceFeature(&createInfo, &deviceFeatures2);
    chainNextDeviceFeature(&createInfo, &deviceFeatures12);
    chainNextDeviceFeature(&createInfo, &deviceFeatures13);
    if (isMeshShading()) {
        chainNextDeviceFeature(&createInfo, &meshFeatures);
    }
    if (isSynchronization2()) {
        chainNextDeviceFeature(&createInfo, &synchronization2Features);
    }
#   if defined(__APPLE__)
    chainNextDeviceFeature(&createInfo, &portability);
#   endif

    //createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = 0;
    if (engine->presentationMode) {
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }
    createInfo.enabledLayerCount = 0; // no longer used - validation layers handled in kvInstance
    if (false) {
        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bufferDeviceAddressFeatures.pNext = (void*)createInfo.pNext;
        createInfo.pNext = &bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_TRUE;
        bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_TRUE;
    }
    if (engine->isVR()) {
        //engine->vr.initVulkanCreateDevice(createInfo);
    }
    else {
        VkResult res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (res != VK_SUCCESS) {
            if (DEBUG_UTILS_EXTENSION) {
                Error("Enabled VK_EXT_DEBUG_UTILS needs Vulkan Configurator running. Did you start it ? ");
            }
            Error("Device Creation failed.");
        }
    }

    vkGetDeviceQueue(device, familyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    engine->util.debugNameObject((uint64_t)graphicsQueue, VK_OBJECT_TYPE_QUEUE, "MAIN GRAPHICS QUEUE");
    vkGetDeviceQueue(device, familyIndices.transferFamily.value(), 0, &transferQueue);
    engine->util.debugNameObject((uint64_t)transferQueue, VK_OBJECT_TYPE_QUEUE, "TRANSFER QUEUE");
    // we normally rely on 2 different queues for multithreading (mainly global updates and render threads)
    // enable single queue rendering with a warning
    if (graphicsQueue == transferQueue) {
        engine->setSingleQueueMode();
        Log("WARNING: Your device does only offer a single queue. Expect severe performance penalties\n");
    }
    if (TRUE) {
        //engine->presentation.createPresentQueue(familyIndices.presentFamily.value());
    }
    if (engine->isVR()) {
        //engine->vr.create();
    }
}

SwapChainSupportDetails GlobalRendering::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details{};
    if (!engine->presentationMode) return details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

void GlobalRendering::createCommandPool(VkCommandPool& pool, std::string name)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        Error("failed to create command pool!");
    }
    if (!name.empty()) {
        engine->util.debugNameObjectCommandPool(pool, name.c_str());
    }
}

void GlobalRendering::createCommandPoolTransfer(VkCommandPool& pool)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndices.transferFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        Error("failed to create transfer command pool!");
    }
}

void GlobalRendering::createCommandPools()
{
    createCommandPool(commandPool);
    createCommandPoolTransfer(commandPoolTransfer);
    workerThreadResources.resize(engine->numWorkerThreads); // Initialize the vector with the appropriate size
    for (int i = 0; i < engine->numWorkerThreads; i++) {
        //workerThreadResources[i].threadResourcesIndex = i;
        createCommandPool(workerThreadResources[i].commandPool, engine->util.createDebugName("WorkerThreadCommandPool_", i));
    }
}

VkCommandBuffer GlobalRendering::beginSingleTimeCommands(bool sync, QueueSelector queue) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    if (queue == QueueSelector::GRAPHICS) {
        allocInfo.commandPool = commandPool;
    } else if (queue == QueueSelector::TRANSFER) {
        allocInfo.commandPool = commandPoolTransfer;
    }
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{};
    engine->util.debugNameObjectCommandBuffer(commandBuffer, "SINGLE TIME COMMAND BUFFER");
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void GlobalRendering::endSingleTimeCommands(VkCommandBuffer commandBuffer, bool sync, QueueSelector queue, uint64_t flags) {
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (flags & QUEUE_FLAG_PERMANENT_UPDATE) {
        Log("submit single command via graphics queue")
    } else if (queue == QueueSelector::GRAPHICS) {
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    } else if (queue == QueueSelector::TRANSFER) {
        //Log("submit via transfer queue\n");
        vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
        //Log("submit via transfer queue END\n");
        vkQueueWaitIdle(transferQueue);
        vkFreeCommandBuffers(device, commandPoolTransfer, 1, &commandBuffer);
    }
}

void GlobalRendering::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, string bufferDebugName) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        Error("failed to create buffer!");
    }
    //Log("createBuffer: " << hex << buffer << endl);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        Error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    engine->util.debugNameObjectBuffer(buffer, bufferDebugName.c_str());
    string memName = bufferDebugName + " memory";
    engine->util.debugNameObjectDeviceMmeory(bufferMemory, memName.c_str());
}

void GlobalRendering::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, QueueSelector queue, uint64_t flags) {
    auto commandBuffer = beginSingleTimeCommands(false, queue);
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer, false, queue);
}

void GlobalRendering::uploadBuffer(VkBufferUsageFlagBits usage, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory,
    string bufferDebugName, QueueSelector queue, uint64_t flags)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory, bufferDebugName + " Staging");

    void* data;
    vkMapMemory(engine->globalRendering.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, src, (size_t)bufferSize);
    vkUnmapMemory(engine->globalRendering.device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
        buffer, bufferMemory, bufferDebugName);

    //for (int i = 0; i < 10000; i++)
    engine->globalRendering.copyBuffer(stagingBuffer, buffer, bufferSize, queue);

    vkDestroyBuffer(engine->globalRendering.device, stagingBuffer, nullptr);
    vkFreeMemory(engine->globalRendering.device, stagingBufferMemory, nullptr);
    //vkDestroyBuffer(engine->global.device, buffer, nullptr);
    //vkFreeMemory(engine->global.device, bufferMemory, nullptr);
}

void GlobalRendering::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = physicalDeviceProperties.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    // float values mean mip level: 0.0 is most detailed e.g. 10.0 is single pixel if there are 11 mip levels
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    //samplerInfo.minLod = 6.0f;
    //samplerInfo.maxLod = 6.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler[(int)TextureType::TEXTURE_TYPE_MIPMAP_IMAGE]) != VK_SUCCESS) {
        Error("failed to create texture sampler TEXTURE_TYPE_MIPMAP_IMAGE!");
    }

    // change for heightmaps:
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // makes border color irrelevant
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // this: do not interpolate heightmap values
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler[(int)TextureType::TEXTURE_TYPE_HEIGHT]) != VK_SUCCESS) {
        Error("failed to create texture sampler TEXTURE_TYPE_HEIGHT!");
    }
}

VkImageView GlobalRendering::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        Error("failed to create texture image view!");
    }

    return imageView;
}

VkImageView GlobalRendering::createImageViewCube(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        Error("failed to create texture image view!");
    }

    return imageView;
}

void GlobalRendering::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                  VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, const char* debugName, uint32_t layers) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = layers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (layers == 6) {
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    else {
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
    }

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        Error("failed to create image!");
    }
    engine->util.debugNameObjectImage(image, debugName);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = engine->globalRendering.findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        Error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
    string memName = debugName;
    memName += "_dev_mem";
    engine->util.debugNameObjectDeviceMmeory(imageMemory, memName.c_str());
}

void GlobalRendering::createCubeMapFrom2dTexture(string textureName2d, string textureNameCube, TextureStore* textureStore)
{
    FrameBufferAttachment attachment{};
    assert(false);
    TextureInfo* twoD = nullptr;//textureStore->getTexture(textureName2d);

    createImageCube(twoD->vulkanTexture.width, twoD->vulkanTexture.height, twoD->vulkanTexture.levelCount, VK_SAMPLE_COUNT_1_BIT, twoD->vulkanTexture.imageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        attachment.image, attachment.memory, textureNameCube.c_str());
    auto cmd = beginSingleTimeCommands(true);

    auto subresourceRangeSrc = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, twoD->vulkanTexture.levelCount, 0, 1 };
    auto subresourceRangeDest = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, twoD->vulkanTexture.levelCount, 0, 6 };

    // Transition destination image to transfer destination layout
    VkImageMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = 0;
    dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstBarrier.image = attachment.image;
    dstBarrier.subresourceRange = subresourceRangeDest;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

    // Transition source image from LAYOUT_SHADER_READ_ONLY_OPTIMAL to transfer destination layout
    VkImageMemoryBarrier srcBarrier{};
    srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    srcBarrier.srcAccessMask = 0;
    srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    srcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    srcBarrier.image = twoD->vulkanTexture.image;
    srcBarrier.subresourceRange = subresourceRangeSrc;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &srcBarrier);

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.depth = 1;

    // copy all layers
    for (uint32_t layer = 0; layer < 6; layer++) {
        imageCopyRegion.dstSubresource.baseArrayLayer = layer;
        // copy all mips:
        for (uint32_t mip = 0; mip < twoD->vulkanTexture.levelCount; mip++) {
            imageCopyRegion.srcSubresource.mipLevel = mip;
            imageCopyRegion.dstSubresource.mipLevel = mip;
            imageCopyRegion.extent.width = twoD->vulkanTexture.width >> mip;
            imageCopyRegion.extent.height = twoD->vulkanTexture.height >> mip;
            vkCmdCopyImage(
                cmd,
                twoD->vulkanTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageCopyRegion);

        }
    }
    // Transition destination image to LAYOUT_SHADER_READ_ONLY_OPTIMAL
    dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dstBarrier.image = attachment.image;
    dstBarrier.subresourceRange = subresourceRangeDest;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

    // Transition source image back to LAYOUT_SHADER_READ_ONLY_OPTIMAL
    srcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    srcBarrier.srcAccessMask = 0;
    srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    srcBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    srcBarrier.image = twoD->vulkanTexture.image;
    srcBarrier.subresourceRange = subresourceRangeSrc;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &srcBarrier);

    endSingleTimeCommands(cmd);

    assert(false);
    ::TextureInfo* texture = nullptr; // textureStore->createTextureSlot(textureNameCube);
    // copy base ktx texture fields and the adapt for new cube map:
    texture->vulkanTexture = twoD->vulkanTexture;
    texture->vulkanTexture.deviceMemory = nullptr;
    texture->vulkanTexture.layerCount = 6;
    texture->vulkanTexture.image = attachment.image;
    texture->vulkanTexture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    texture->vulkanTexture.deviceMemory = attachment.memory;
    texture->isKtxCreated = false;
    texture->imageView = createImageViewCube(texture->vulkanTexture.image, texture->vulkanTexture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
    texture->available = true;
}

void GlobalRendering::destroyImage(VkImage image, VkDeviceMemory imageMemory)
{
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);
}

void GlobalRendering::destroyImageView(VkImageView imageView)
{
    vkDestroyImageView(device, imageView, nullptr);
}

void GlobalRendering::createViewportState(ShaderState& shaderState) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)engine->getBackBufferExtent().width;
    viewport.height = (float)engine->getBackBufferExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = engine->getBackBufferExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &shaderState.viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &shaderState.scissor;

    shaderState.viewport = viewport;
    shaderState.scissor = scissor;
    shaderState.viewportState = viewportState;
}

void GlobalRendering::logDeviceLimits()
{
    Log("maxUniformBufferRange (width of one buffer) " << physicalDeviceProperties.properties.limits.maxUniformBufferRange << endl);
    Log("minMemoryMapAlignment " << physicalDeviceProperties.properties.limits.minMemoryMapAlignment << endl);
    Log("minUniformBufferOffsetAlignment " << physicalDeviceProperties.properties.limits.minUniformBufferOffsetAlignment << endl);
    Log("maxDescriptorSetSampledImages " << physicalDeviceProperties.properties.limits.maxDescriptorSetSampledImages << endl);
    Log("maxPushConstantsSize " << physicalDeviceProperties.properties.limits.maxPushConstantsSize << endl);
    // maxDescriptorSetSampledImages
}

GPUImage* GlobalRendering::createImage(vector<GPUImage>& list, const char *debugName, uint32_t width, uint32_t height)
{
    GPUImage gpui;
    if (width <= 0 || height <= 0) {
        width = engine->getBackBufferExtent().width;
        height = engine->getBackBufferExtent().height;
    }
    gpui.width = width;
    gpui.height = height;
    gpui.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    gpui.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    gpui.rendered = false;
    gpui.consumed = false;
    createImage(gpui.width, gpui.height, 1, VK_SAMPLE_COUNT_1_BIT, ImageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpui.fba.image, gpui.fba.memory, debugName);
    gpui.fba.view = createImageView(gpui.fba.image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    list.push_back(gpui);
    return &list.back();
}

void GlobalRendering::createDumpImage(GPUImage& gpui, uint32_t width, uint32_t height)
{
    assert(gpui.fba.image == nullptr);
    if (width <= 0 || height <= 0) {
        width = engine->getBackBufferExtent().width;
        height = engine->getBackBufferExtent().height;
    }
    gpui.width = width;
    gpui.height = height;
    createImage(gpui.width, gpui.height, 1, VK_SAMPLE_COUNT_1_BIT, ImageFormat, VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        gpui.fba.image, gpui.fba.memory, "dump image");
    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkGetImageSubresourceLayout(device, gpui.fba.image, &subResource, &gpui.subResourceLayout);

    // Map image memory so we can start copying from it
    vkMapMemory(device, gpui.fba.memory, 0, VK_WHOLE_SIZE, 0, (void**)&gpui.imagedata);
    gpui.imagedata += gpui.subResourceLayout.offset;
}

void GlobalRendering::consolidateCommandBuffers(CommandBufferArray& cmdBufs, FrameResources* fr)
{
    assert(fr->numCommandBuffers > 0);
    assert(fr->numCommandBuffers < MAX_COMMAND_BUFFERS_PER_DRAW);
    int index = 0;
    fr->forEachCommandBuffer([&index, &cmdBufs](VkCommandBuffer b) {
        //Log("cmd buffer " << b << endl);
        cmdBufs[index++] = b;
        });
}

void GlobalRendering::makeGPUImage(FrameBufferAttachment* fba, GPUImage& gpui)
{
    gpui.width = engine->getBackBufferExtent().width;
    gpui.height = engine->getBackBufferExtent().height;
    gpui.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    gpui.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    gpui.rendered = true;
    gpui.consumed = false;
    gpui.fba = *fba;
}

void GlobalRendering::dumpToFile(FrameBufferAttachment* fba, DirectImage& di)
{
    GPUImage gpui;
    makeGPUImage(fba, gpui);
    di.dumpToFile(&gpui);
}

void GlobalRendering::present(FrameResources* fr, DirectImage& di, WindowInfo* winfo)
{
    //GPUImage* gpui = &fr->colorImage;
    engine->presentation.presentImage(fr, winfo, &fr->colorImage);
    //auto commandBuffer = beginSingleTimeCommands(false);
    //di.copyBackbufferImage(gpui, &target, commandBuffer);
    //endSingleTimeCommands(commandBuffer);
}

void GlobalRendering::preFrame(FrameResources* fr)
{
    // wait for fence signal from submit call 2 frames before
    LogCondF(LOG_QUEUE, "wait present fence image index " << fr->frameIndex << endl);
    //VkResult fenceStatus = vkGetFenceStatus(device, fr->inFlightFence);
    //if (fenceStatus == VK_SUCCESS) {
    //    //Log("fence signaled image index " << fr->frameIndex << endl);
    //} else {
    //    Log("fence not signaled image index " << fr->frameIndex << endl);
    //}
    //Log("wait for fence " << fr->inFlightFence << " index " << fr->frameIndex << endl);
    //vkWaitForFences(device, 1, &fr->inFlightFence, VK_TRUE, UINT64_MAX);
    //vkResetFences(device, 1, &fr->inFlightFence);
    // wait until old image from 2 frames before is processed
    //auto v = fr->processImageQueue.pop();
    LogCondF(LOG_QUEUE, "signalled present fence image index " << fr->frameIndex << endl);
}

void GlobalRendering::postFrame(FrameResources* fr)
{
}

// submit to graphics queue, called only from queue submit thread
// 1) finalize command buffers
// 2) wait for last frame to be processed (last submit has finished)
// 3) submit new command buffers
// 4) call preocessImage to process last frame
void GlobalRendering::submit(FrameResources* fr)
{
    //Log("submit thread submitting frame " << fr->frameNum << endl);
    consolidateCommandBuffers(submitCommandBuffers, fr);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //VkSemaphore waitSemaphores[] = { tr.imageAvailableSemaphore };
    //VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    //tr.waitStages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;//waitSemaphores;
    //submitInfo.pWaitDstStageMask = &tr.waitStages[0];
    submitInfo.commandBufferCount = static_cast<uint32_t>(fr->numCommandBuffers);
    submitInfo.pCommandBuffers = &submitCommandBuffers[0];
    //VkSemaphore signalSemaphores[] = { tr.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr; // signalSemaphores;
    // wait for last submit to finish:
    vkWaitForFences(device, 1, &queueSubmitFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &queueSubmitFence);
    // submit next frame
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, queueSubmitFence) != VK_SUCCESS) {
        Error("failed to submit draw command buffer!");
    }
    //Log("submit fence " << fr->inFlightFence << " index " << fr->frameIndex << endl);
    fr->clearDrawResults();
}

void GlobalRendering::processImage(FrameResources* fr)
{
}

void GlobalRendering::destroyImage(GPUImage* image)
{
    destroyImageView(image->fba.view);
    destroyImage(image->fba.image, image->fba.memory);
}
