#pragma once

// forward declarations
class ShadedPathEngine;

// global resources that are not changed in rednering threads.
// shader code, meshes, etc.
class GlobalRendering
{
private:
	// we need direct access to engine instance
	ShadedPathEngine& engine;

	// for now we just assemble all global shader resources here
	VkShaderModule vertShaderModuleTriangle = nullptr;
	VkShaderModule fragShaderModuleTriangle = nullptr;


public:
	GlobalRendering(ShadedPathEngine& s) : engine(s) {
		// initializations
	};
	// detroy global resources, should only be called from engine dtor
	void destroy();
	~GlobalRendering();
	void initiateShader_Triangle();
	Files files;
	VkShaderModule createShaderModule(const vector<byte>& code);
};

