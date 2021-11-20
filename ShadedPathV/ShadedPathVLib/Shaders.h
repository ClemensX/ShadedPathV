#pragma once
class Shaders
{
public:
	Shaders(ShadedPathEngine& s) : engine(s) {
		Log("Shaders c'tor\n");
	};
	~Shaders() {
		Log("Shaders destructor\n");
	};
	void initiateShader_Triangle();
	void drawFrame_Triangle();
	bool shouldClose();
	VkShaderModule createShaderModule(const vector<byte>& code);
	void recordDrawCommand_Triangle(VkCommandBuffer& commandBuffer, ThreadResources &tr);

private:
	void initiateShader_TriangleSingle(ThreadResources &res);
	ShadedPathEngine& engine;
	VkShaderModule vertShaderModuleTriangle = nullptr;
	VkShaderModule fragShaderModuleTriangle = nullptr;
};

