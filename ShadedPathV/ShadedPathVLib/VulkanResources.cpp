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
    uint32_t bindingCount = 0;

    for (auto& d : def) {
        addSetLayoutBinding(d);
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(engine->global.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
}

void VulkanResources::addSetLayoutBinding(VulkanResourceElement el)
{
    uint32_t bindingCount = bindings.size();
    VkDescriptorSetLayoutBinding layoutBinding{};
    if (el.type == VulkanResourceType::MVPBuffer) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
    } else if (el.type == VulkanResourceType::SingleTexture) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
    }
}

VulkanResources::~VulkanResources()
{
	Log("VulkanResources destructor\n");
}