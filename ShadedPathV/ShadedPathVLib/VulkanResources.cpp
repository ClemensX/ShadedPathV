#include "pch.h"

using namespace std;

void VulkanResources::init(ShadedPathEngine* engine) {
	Log("VulkanResources c'tor\n");
	this->engine = engine;
}

VkShaderModule VulkanResources::createShaderModule(string filename)
{
    vector<byte> file_buffer;
    engine->files.readFile(filename, file_buffer, FileCategory::FX);
    //Log("read shader: " << file_buffer.size() << endl);
    // create shader modules
    VkShaderModule shaderModule = engine->shaders.createShaderModule(file_buffer);
    return shaderModule;
}


VulkanResources::~VulkanResources()
{
	Log("VulkanResources destructor\n");
}