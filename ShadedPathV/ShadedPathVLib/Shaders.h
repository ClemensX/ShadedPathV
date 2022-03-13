#pragma once
class Config {
public:
	Config& add(ShaderBase &shader) {
		shaderList.push_back(&shader);
		return *this;
	}

	Config& init() {
		for (ShaderBase *shader : shaderList) {
			shader->init(*engine, shaderState);
		}
		return *this;
	}

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
	void initiateShader_Triangle();
	void createCommandBufferTriangle(ThreadResources& tr);
	void recordDrawCommand_Triangle(VkCommandBuffer& commandBuffer, ThreadResources& tr);
	// draw triangle during frame creation
	void drawFrame_Triangle(ThreadResources& tr);

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
	VkShaderModule vertShaderModuleTriangle = nullptr;
	VkShaderModule fragShaderModuleTriangle = nullptr;
	VkBuffer vertexBufferTriangle = nullptr;
	VkDeviceMemory vertexBufferMemoryTriangle = nullptr;
	VkBuffer indexBufferTriangle = nullptr;
	VkDeviceMemory indexBufferMemoryTriangle = nullptr;

	unsigned int imageCouter = 0;
	bool enabledTriangle = false;
	bool enabledImageDump = false;
};

