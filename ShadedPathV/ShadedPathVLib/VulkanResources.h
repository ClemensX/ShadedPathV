#pragma once

// forward declarations
class ShadedPathEngine;

// all resources needed for running in a separate thread.
// it is ok to also READ from GlobalRendering and engine
class VulkanResources
{
public:
	// init texture store
	void init(ShadedPathEngine* engine);

	~VulkanResources();

	// create shader module
	VkShaderModule createShaderModule(std::string filename);

private:
	ShadedPathEngine* engine = nullptr;
};
