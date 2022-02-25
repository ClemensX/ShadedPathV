#include "pch.h"

void LineShader::init(ShadedPathEngine& engine)
{
	this->device = engine.global.device;
	this->global = &engine.global;
	this->engine = &engine;
}

void LineShader::add(vector<LineDef>& linesToAdd)
{
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
	dirty = true;
}

void LineShader::initialUpload()
{
	// create vertex buffer
	VkDeviceSize bufferSize = sizeof(Vertex) * lines.size() * 2; // we need 2 vertices per line to draw
	global->uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, lines.data(), vertexBuffer, vertexBufferMemory);

}

LineShader::~LineShader()
{
	Log("LineShader destructor\n");
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}