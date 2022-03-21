#include "pch.h"

void ShaderBase::init(ShadedPathEngine& engine)
{
	this->device = engine.global.device;
	this->global = &engine.global;
	this->engine = &engine;
	enabled = true;
}

VkShaderModule ShaderBase::createShaderModule(const vector<byte>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(engine->global.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		Error("failed to create shader module!");
	}
	return shaderModule;
}

void ShaderBase::createUniformBuffer(ThreadResources& res, VkBuffer &uniformBuffer, size_t size, VkDeviceMemory &uniformBufferMemory)
{
	VkDeviceSize bufferSize = size;
	// we do not want to handle zero length buffers
	if (bufferSize == 0) {
		stringstream s;
		s << "Cannot create Uniform buffer with size " << bufferSize << ". (Did you forget to subclass buffer definition ? )" << endl;
		Error(s.str());
	}
	global->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer, uniformBufferMemory);
}

void ShaderBase::createDescriptorPool(vector<VkDescriptorPoolSize> &poolSizes)
{
	// duplicate the poolSize for every render thread we have:
	vector<VkDescriptorPoolSize> allThreadsPoolSizes;
	for (int i = 0; i < engine->getFramesInFlight(); i++) {
		copy(poolSizes.begin(), poolSizes.end(), back_inserter(allThreadsPoolSizes));
	}
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(allThreadsPoolSizes.size());
	poolInfo.pPoolSizes = allThreadsPoolSizes.data();
	poolInfo.maxSets = 5; // arbitrary number for now TODO: see if this can be calculated
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		Error("failed to create descriptor pool!");
	}
}

ShaderBase::~ShaderBase() {

}
