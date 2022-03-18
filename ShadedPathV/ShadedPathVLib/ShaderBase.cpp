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

ShaderBase::~ShaderBase() {

}
