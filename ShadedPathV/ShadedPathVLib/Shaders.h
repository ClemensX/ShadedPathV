#pragma once
class Shaders
{
public:
	Shaders(ShadedPathEngine& s) : engine(s) {
		Log("Shaders c'tor\n");
	};
	~Shaders();
	void initiateShader_Triangle();
	void initiateShader_BackBufferImageDump();
	void drawFrame_Triangle(ThreadResources& tr);
	VkShaderModule createShaderModule(const vector<byte>& code);
	void recordDrawCommand_Triangle(VkCommandBuffer& commandBuffer, ThreadResources &tr);
	void executeBufferImageDump(ThreadResources& tr);

private:
	void initiateShader_TriangleSingle(ThreadResources &res);
	void initiateShader_BackBufferImageDumpSingle(ThreadResources& res);
	ShadedPathEngine& engine;
	VkShaderModule vertShaderModuleTriangle = nullptr;
	VkShaderModule fragShaderModuleTriangle = nullptr;
	unsigned int imageCouter = 0;
	bool enabledTriangle = false;
	bool enabledImageDump = false;
};

