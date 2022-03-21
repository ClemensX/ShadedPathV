#pragma once

class Config {
public:
	// add next shader to list of shaders
	// shaders will be called in the order they were added here.
	Config& add(ShaderBase &shader) {
		shaderList.push_back(&shader);
		return *this;
	}

	// Initialize ShaderState and all shaders
	Config& init();

	void setEngine(ShadedPathEngine* s) {
		engine = s;
	}

private:
	vector<ShaderBase*> shaderList;
	ShadedPathEngine* engine;
	ShaderState shaderState;
};

class Shaders
{
public:
	Shaders(ShadedPathEngine& s) : engine(s) {
		Log("Shaders c'tor\n");
		config.setEngine(&s);
	};
	~Shaders();

	Config config;

	// general methods
	void queueSubmit(ThreadResources& tr);
	VkShaderModule createShaderModule(const vector<byte>& code);

	// SHADERS

	// Triangle shader (most simple beginner shader - just one triangle, no MVP necessary)
	// initiate before rendering first frame
	SimpleShader simpleShader;
	LineShader lineShader;
	// submit command buffers for current frame
	void submitFrame(ThreadResources& tr);

	// BackBufferImageDump shader: copy backbuffer to image file
	// initiate before rendering first frame
	void initiateShader_BackBufferImageDump();
	// write backbuffer image to file (during frame creation)
	void executeBufferImageDump(ThreadResources& tr);

	// UI shader
	void createCommandBufferUI(ThreadResources& tr);

private:
	void initiateShader_TriangleSingle(ThreadResources &res);
	void initiateShader_BackBufferImageDumpSingle(ThreadResources& res);
	ShadedPathEngine& engine;

	unsigned int imageCouter = 0;
	bool enabledImageDump = false;
};

