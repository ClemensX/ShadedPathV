#include "pch.h"

using namespace std;

void VulkanResources::init(ShadedPathEngine* engine) {
	Log("VulkanResources c'tor\n");
	this->engine = engine;
}

VkShaderModule VulkanResources::createShaderModule(string filename)
{
    vector<byte> file_buffer;
    engine->files.readFile(filename, file_buffer, FileCategory::FX);
    //Log("read shader: " << file_buffer.size() << endl);
    // create shader modules
    VkShaderModule shaderModule = engine->shaders.createShaderModule(file_buffer);
    return shaderModule;
}

void VulkanResources::createVertexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    vector<VulkanResourceElement> &def = *resourceDefinition;
    assert(def[bufferId].type == VulkanResourceType::VertexBufferStatic);
    this->engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory);
}

void VulkanResources::createIndexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;
    assert(def[bufferId].type == VulkanResourceType::IndexBufferStatic);
    this->engine->global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory);
}

void VulkanResources::createDescriptorSetResources(VkDescriptorSetLayout& layout, VkDescriptorPool& pool)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;

    for (auto& d : def) {
        addResourcesForElement(d);
    }

    //VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    //VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info{};
    //flag_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    //flag_info.bindingCount = static_cast<uint32_t>(bindings.size());
    //flag_info.pBindingFlags = &flag;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //layoutInfo.pNext = &flag_info;
    layoutInfo.pNext = nullptr;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(engine->global.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = engine->getFramesInFlight(); // max num that can be allocated
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(engine->global.device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        Error("failed to create descriptor pool!");
    }
    this->layout = layout;
    this->pool = pool;
}

void VulkanResources::addResourcesForElement(VulkanResourceElement el)
{
    uint32_t bindingCount = bindings.size();
    VkDescriptorSetLayoutBinding layoutBinding{};
    VkDescriptorPoolSize poolSize{};
    if (el.type == VulkanResourceType::MVPBuffer) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);
    } else if (el.type == VulkanResourceType::SingleTexture) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);
    } else if (el.type == VulkanResourceType::GlobalTextureSet) {
        createDescriptorSetResourcesForTextures();
    }
}

void VulkanResources::createThreadResources(VulkanHandoverResources& hdv)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;
    if (hdv.descriptorSet == nullptr) Error("Shader did not initialize needed Thread Resources: descriptorSet is missing");
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    if (vkAllocateDescriptorSets(engine->global.device, &allocInfo, hdv.descriptorSet) != VK_SUCCESS) {
        Error("failed to allocate descriptor sets!");
    }

    for (auto& d : def) {
        addThreadResourcesForElement(d, hdv);
    }
    vkUpdateDescriptorSets(engine->global.device, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
    descriptorSets.clear();
    bufferInfos.clear();
    imageInfos.clear();
}

void VulkanResources::addThreadResourcesForElement(VulkanResourceElement el, VulkanHandoverResources& hdv)
{
    VkWriteDescriptorSet descSet{};
    if (hdv.descriptorSet == nullptr) Error("Shader did not initialize needed Thread Resources: descriptorSet is missing");

    if (el.type == VulkanResourceType::MVPBuffer) {
        if (hdv.mvpBuffer == nullptr) Error("Shader did not initialize needed Thread Resources: mvpBuffer is missing");
        if (hdv.mvpSize == 0L) Error("Shader did not initialize needed Thread Resources: mvpSize is missing");
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = hdv.mvpBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = hdv.mvpSize;
        bufferInfos.push_back(bufferInfo);

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 0;
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descSet.descriptorCount = 1;
        descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];

        descriptorSets.push_back(descSet);
    }
    else if (el.type == VulkanResourceType::SingleTexture) {
        if (hdv.imageView == nullptr) Error("Shader did not initialize needed Thread Resources: imageView is missing");
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = hdv.imageView;
        imageInfo.sampler = engine->global.textureSampler;
        imageInfos.push_back(imageInfo);

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 1;
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descSet.descriptorCount = 1;
        descSet.pImageInfo = &imageInfos[imageInfos.size() - 1];

        descriptorSets.push_back(descSet);
    }
}

void VulkanResources::createDescriptorSetResourcesForTextures()
{
    // check if already initialized
    if (engine->textureStore.layout != nullptr) return;

    VkDescriptorSetLayoutBinding layoutBinding{};
    VkDescriptorPoolSize poolSize{};

    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = engine->textureStore.getMaxSize();
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    layoutBinding.pImmutableSamplers = nullptr;
    textureBindings.push_back(layoutBinding);
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = layoutBinding.descriptorCount;
    texturePoolSizes.push_back(poolSize);

    VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info{};
    flag_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flag_info.bindingCount = 1;
    flag_info.pBindingFlags = &flag;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &flag_info;
    layoutInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
    layoutInfo.pBindings = textureBindings.data();

    if (vkCreateDescriptorSetLayout(engine->global.device, &layoutInfo, nullptr, &engine->textureStore.layout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout for textures!");
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(texturePoolSizes.size());
    poolInfo.pPoolSizes = texturePoolSizes.data();
    poolInfo.maxSets = 1; // only one global list for all shaders
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(engine->global.device, &poolInfo, nullptr, &engine->textureStore.pool) != VK_SUCCESS) {
        Error("failed to create descriptor pool for textures!");
    }
}

void VulkanResources::updateDescriptorSetForTextures() {
    if (globalTextureDescriptorSetValid) return;
    // iterate over all textures and write descriptor sets
    for (auto& texMapEntry : engine->textureStore.getTexturesMap()) {
        auto& tex = texMapEntry.second;
        Log("tex: " << tex.id.c_str() << " index: " << tex.index << endl);
    }
    globalTextureDescriptorSetValid = true;
}

void VulkanResources::updateDescriptorSets(ThreadResources& tr)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;
    for (auto& d : def) {
        if (d.type == VulkanResourceType::GlobalTextureSet) {
            updateDescriptorSetForTextures();
        }

    }
}

VulkanResources::~VulkanResources()
{
	Log("VulkanResources destructor\n");
}