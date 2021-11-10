#include "pch.h"

// intialize global shader resources
void GlobalRendering::initiateShader_Triangle()
{
	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	files.readFile("triangle_vert.spv", file_buffer_vert, FileCategory::FX);
	files.readFile("triangle_frag.spv", file_buffer_frag, FileCategory::FX);
	Log("read vertex shader: " << file_buffer_vert.size() << endl);
	Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	vertShaderModuleTriangle = createShaderModule(file_buffer_vert);
	fragShaderModuleTriangle = createShaderModule(file_buffer_frag);
	// create shader stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModuleTriangle;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModuleTriangle;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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

void GlobalRendering::destroy()
{
	// destroy 
	vkDestroyShaderModule(engine.device, fragShaderModuleTriangle, nullptr);
	vkDestroyShaderModule(engine.device, vertShaderModuleTriangle, nullptr);
}

GlobalRendering::~GlobalRendering()
{
}