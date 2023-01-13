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

void VulkanResources::createVertexBufferStatic(std::vector<VulkanResourceElement>& def, size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    assert(def[bufferId].type == VulkanResourceType::VertexBufferStatic);
    this->engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory);
}

void VulkanResources::createIndexBufferStatic(std::vector<VulkanResourceElement>& def, size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    assert(def[bufferId].type == VulkanResourceType::IndexBufferStatic);
    this->engine->global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory);
}

VulkanResources::~VulkanResources()
{
	Log("VulkanResources destructor\n");
}