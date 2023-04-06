#include "pch.h"

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

void GlobalRendering::initBeforePresentation()
{
    gatherDeviceExtensions();
    initVulkanInstance();
}

void GlobalRendering::initAfterPresentation()
{
    // list available devices:
    pickPhysicalDevice(true);
    // pick device
    pickPhysicalDevice();
    // list queue properties:
    findQueueFamilies(physicalDevice, true);
    createLogicalDevice();
    createCommandPool();
    createTextureSampler();
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // binary is default

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &singleTimeCommandsSemaphore) != VK_SUCCESS) {
        Error("failed to create singleTimeCommandsSemaphore");
    }
}

void GlobalRendering::shutdown()
{
    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroySemaphore(device, singleTimeCommandsSemaphore, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    device = nullptr;
    vkDestroyInstance(vkInstance, nullptr);
    vkInstance = nullptr;
    glfwTerminate();
}

void GlobalRendering::gatherDeviceExtensions()
{
    engine.presentation.possiblyAddDeviceExtensions(deviceExtensions);
}

void GlobalRendering::initVulkanInstance()
{
    if (!checkProfileSupport()) {
        Error("required vulkan profile not available!");
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ShadedPathV";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ShadedPathV";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VP_KHR_ROADMAP_2022_MIN_API_VERSION;//API_VERSION;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (USE_PROFILE_DYN_RENDERING) {
        VpInstanceCreateInfo vpCreateInfo{};
        vpCreateInfo.pCreateInfo = &createInfo;
        vpCreateInfo.pProfile = &profile;
        vpCreateInfo.flags = VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT;

        if (vpCreateInstance(&vpCreateInfo, nullptr, &vkInstance) != VK_SUCCESS) {
            Error("failed to create instance!");
        }
    }
    else {
        vkInstance = VK_NULL_HANDLE;
        if (engine.isVR()) {
            engine.vr.initVulkanEnable2(createInfo);
        } else {
            if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS) {
                Error("failed to create instance!");
            }
        }
    }

    // list available extensions:
    if (LIST_EXTENSIONS) {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availExtensions.data());

        Log("available Vulkan instance extensions:\n");
        for (const auto& extension : availExtensions) {
            Log("  " << extension.extensionName << '\n');
        }
    }
}

std::vector<const char*> GlobalRendering::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // vector will be moved on return
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    //extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    //extensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    //extensions.push_back(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
    if (DEBUG_UTILS_EXTENSION) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    if (engine.isMeshShading()) {
        deviceExtensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
    }

    Log("requested Vulkan instance extensions:" << endl)
        Util::printCStringList(extensions);
    Log("requested Vulkan device extensions:" << endl)
        Util::printCStringList(deviceExtensions);

    return extensions;
}

void GlobalRendering::pickPhysicalDevice(bool listmode)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        Error("no physical vulkan device available");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, listmode)) {
            // check for same device (phys device may have been initialized by OpenXR
            if (physicalDevice != nullptr) {
                if (physicalDevice != device) {
                    Error("Physical Device selected does not match pre-selected device from OpenXR");
                } else {
                    // all is fine - er already have our phys device
                    return;
                }
            }
            physicalDevice = device;
            break;
        }
    }
    if (listmode)
        return;
    if (physicalDevice == VK_NULL_HANDLE) {
        Error("failed to find a suitable GPU!");
    }
}

bool GlobalRendering::isDeviceSuitable(VkPhysicalDevice device, bool listmode)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    // query extension details (mesh shader)
    VkPhysicalDeviceMeshShaderPropertiesNV meshProperties = {};
    meshProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
    meshProperties.pNext = nullptr;
    VkPhysicalDeviceProperties2 deviceProperties2 = {};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &meshProperties;
    vkGetPhysicalDeviceProperties2(device, &deviceProperties2);
    physicalDeviceProperties = deviceProperties2;

    if (listmode) {
        Log("Physical Device properties: " << deviceProperties.deviceName << " Vulkan API Version: " << Util::decodeVulkanVersion(deviceProperties.apiVersion).c_str() << " type: " << Util::decodeDeviceType(deviceProperties.deviceType) << endl);
        checkDeviceExtensionSupport(device, listmode);
        return false;
    }
    // we just pick the first device for now
    Log("picked physical device: " << deviceProperties.deviceName << " " << Util::decodeVulkanVersion(deviceProperties.apiVersion).c_str() << endl);

    // now look for queue families:
    familyIndices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device, false);
    bool swapChainAdequate = false;
    if (engine.presentation.enabled) {
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
    }
    else {
        swapChainAdequate = true;
    }
    // check mesh support:
    // set extension details for mesh shader
    VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
    meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
    meshFeatures.pNext = nullptr;
    VkPhysicalDeviceFeatures2 feature2{};
    feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feature2.pNext = &meshFeatures;
    vkGetPhysicalDeviceFeatures2(device, &feature2);
    VkPhysicalDeviceMeshShaderFeaturesNV* meshSupport = (VkPhysicalDeviceMeshShaderFeaturesNV*)feature2.pNext;
    if (engine.isMeshShading()) {
        //Log(meshSupport << endl);
        if (!meshSupport->meshShader || !meshSupport->taskShader) {
            Log("device does not support needed Mesh Shader" << endl);
            return false;
        }
    }
    // check compressed texture support:
    VkFormatProperties fp{};
    vkGetPhysicalDeviceFormatProperties(device, VK_FORMAT_BC7_SRGB_BLOCK, &fp);
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

    // check descrtiptor index size suitable for texture count:
    if (physicalDeviceProperties.properties.limits.maxDescriptorSetSampledImages < TextureStore::UPPER_LIMIT_TEXTURE_COUNT) {
        Log("Device does not support enough texture slots in descriptors" << endl);
        return false;
    }
    return familyIndices.isComplete(engine.presentation.enabled) && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy && deviceFeatures.textureCompressionBC;
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
                Log("found graphics queue, max queues: " << queueFamily.queueCount << endl);
            }
        }
        if (engine.presentation.enabled) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, engine.presentation.surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }
        if (!listmode && indices.isComplete(engine.presentation.enabled)) {
            break;
        }
        i++;
    }
    return indices;
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


void GlobalRendering::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value() };
    if (engine.presentation.enabled) {
        uniqueQueueFamilies.insert({ indices.presentFamily.value() });
    }

    float queuePriority[] = {1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f};
    //float queuePriority[] {1.0f};
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 16;
        queueCreateInfo.pQueuePriorities = &queuePriority[0];
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        // provoke validation layer warning by commenting out following line:
        .geometryShader = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .textureCompressionBC = VK_TRUE,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
        //deviceFeatures.textureCompressionETC2 = VK_TRUE; not supported on Quadro P2000 with Max-Q Design 1.3.194
        //deviceFeatures.textureCompressionASTC_LDR = VK_TRUE; not supported on Quadro P2000 with Max-Q Design 1.3.194
        //deviceFeatures.dynamicRendering
    };

    // set extension details for mesh shader
    VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
    meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
    meshFeatures.meshShader = VK_TRUE;
    meshFeatures.taskShader = VK_FALSE;
    meshFeatures.pNext = nullptr;

    VkPhysicalDeviceVulkan12Features deviceFeatures12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 deviceFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = (void*)&deviceFeatures12,
        .features = deviceFeatures,
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    if (engine.isMeshShading()) {
        createInfo.pNext = &meshFeatures;
    }
    if (!USE_PROFILE_DYN_RENDERING) {
        if (engine.isMeshShading()) {
            meshFeatures.pNext = (void*)&deviceFeatures2;
        } else {
            //createInfo.pNext = &dynamic_rendering_feature;
            createInfo.pNext = &deviceFeatures2;
        }
        //createInfo.pEnabledFeatures = &deviceFeatures;
    }
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = 0;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0; // no longer used - validation layers handled in kvInstance
    if (USE_PROFILE_DYN_RENDERING) {
        VpDeviceCreateInfo vpCreateInfo{};
        vpCreateInfo.pCreateInfo = &createInfo;
        vpCreateInfo.pProfile = &profile;
        vpCreateInfo.flags = VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT;

        checkDeviceProfileSupport(vkInstance, physicalDevice);
        if (vpCreateDevice(physicalDevice, &vpCreateInfo, nullptr, &device) != VK_SUCCESS) {
            Error("failed to create logical device!");
        }
    } else {
        if (engine.isVR()) {
            engine.vr.initVulkanCreateDevice(createInfo);
        } else {
            VkResult res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            if (res != VK_SUCCESS) {
                if (DEBUG_UTILS_EXTENSION) {
                    Error("Enabled VK_EXT_DEBUG_UTILS needs Vulkan Configurator running. Did you start it ? ");
                }
                Error("Device Creation failed.");
            }
        }
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    engine.util.debugNameObject((uint64_t)graphicsQueue, VK_OBJECT_TYPE_QUEUE,  "MAIN GRAPHICS QUEUE");
    if (engine.presentation.enabled) {
        engine.presentation.createPresentQueue(indices.presentFamily.value());
    }
    if (engine.isVR()) {
        engine.vr.createSession();
    }
}

bool GlobalRendering::checkDeviceExtensionSupport(VkPhysicalDevice phys_device, bool listmode)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensionCount, availableExtensions.data());

    if (listmode && LIST_EXTENSIONS) {
        Log("available Vulkan Device extensions:\n");
        for (const auto& extension : availableExtensions) {
            Log("  " << extension.extensionName << '\n');
        }
    }
    VkBool32 supported = false;
    auto result = vpGetPhysicalDeviceProfileSupport(vkInstance, phys_device, &profile, &supported);
    if (result != VK_SUCCESS) {
        Log("Cannot get physical device properties")
        return false;
    }
    else if (supported != VK_TRUE) {
        // profile is not supported at the device level
        return false;
    }
    return true;
}

SwapChainSupportDetails GlobalRendering::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details{};
    if (!engine.presentation.enabled) return details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, engine.presentation.surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine.presentation.surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine.presentation.surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine.presentation.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine.presentation.surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

void GlobalRendering::createCommandPool(VkCommandPool& pool)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        Error("failed to create command pool!");
    }
}

void GlobalRendering::createCommandPool()
{
    createCommandPool(commandPool);
}

VkCommandBuffer GlobalRendering::beginSingleTimeCommands(bool sync) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{};
    engine.util.debugNameObjectCommandBuffer(commandBuffer, "SINGLE TIME COMMAND BUFFER");
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void GlobalRendering::endSingleTimeCommands(VkCommandBuffer commandBuffer, bool sync) {
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void GlobalRendering::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        Error("failed to create buffer!");
    }

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
}

void GlobalRendering::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    auto commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands(commandBuffer);
}

void GlobalRendering::uploadBuffer(VkBufferUsageFlagBits usage, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(engine.global.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, src, (size_t)bufferSize);
    vkUnmapMemory(engine.global.device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
        buffer, bufferMemory);

    //for (int i = 0; i < 10000; i++)
    engine.global.copyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(engine.global.device, stagingBuffer, nullptr);
    vkFreeMemory(engine.global.device, stagingBufferMemory, nullptr);
}

void GlobalRendering::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        Error("failed to create texture sampler!");
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
                                  VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t layers) {
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

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = engine.global.findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        Error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void GlobalRendering::createCubeMapFrom2dTexture(string textureName2d, string textureNameCube)
{
    FrameBufferAttachment attachment{};
    TextureInfo* twoD = engine.textureStore.getTexture(textureName2d);

    createImageCube(twoD->vulkanTexture.width, twoD->vulkanTexture.height, twoD->vulkanTexture.levelCount, VK_SAMPLE_COUNT_1_BIT, twoD->vulkanTexture.imageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        attachment.image, attachment.memory);
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

    ::TextureInfo *texture = engine.textureStore.createTextureSlot(textureNameCube);
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

void GlobalRendering::createViewportState(ShaderState& shaderState) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)engine.getBackBufferExtent().width;
    viewport.height = (float)engine.getBackBufferExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = engine.getBackBufferExtent();

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
    // maxDescriptorSetSampledImages
}
