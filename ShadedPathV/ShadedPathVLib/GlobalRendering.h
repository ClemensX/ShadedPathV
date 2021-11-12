#pragma once

// forward declarations
class ShadedPathEngine;

// global resources that are not changed in rendering threads.
// shader code, meshes, etc.
// all objects here are specific to shaders
// shader independent global objects like framebuffer, swap chain, render passes are in ShadedPathEngine
class GlobalRendering
{
private:
	// we need direct access to engine instance
	ShadedPathEngine& engine;

public:
	GlobalRendering(ShadedPathEngine& s) : engine(s) {
		// initializations
	};
	// detroy global resources, should only be called from engine dtor
	void destroy();
	~GlobalRendering();
	void initiateShader_Triangle();
	void recordDrawCommand_Triangle(VkCommandBuffer &commandBuffer);
	void drawFrame_Triangle();
	Files files;
	VkShaderModule createShaderModule(const vector<byte>& code);
	// for now we just assemble all global shader resources here
	VkShaderModule vertShaderModuleTriangle = nullptr;
	VkShaderModule fragShaderModuleTriangle = nullptr;
	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkSemaphore imageAvailableSemaphoreTriangle;
	VkSemaphore renderFinishedSemaphoreTriangle;
};

