#include "pch.h"

void GlobalRendering::initiateShader_Triangle()
{
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	files.readFile("triangle_vert.spv", file_buffer_vert, FileCategory::FX);
	files.readFile("triangle_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	VkShaderModule vertShaderModule = createShaderModule(file_buffer_vert);
	VkShaderModule fragShaderModule = createShaderModule(file_buffer_frag);

	vkDestroyShaderModule(engine.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(engine.device, vertShaderModule, nullptr);
}

VkShaderModule GlobalRendering::createShaderModule(const vector<byte>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(engine.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		Error("failed to create shader module!");
	}
	return shaderModule;
}