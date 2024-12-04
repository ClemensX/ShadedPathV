#include "mainheader.h"

using namespace std;

DirectImage::DirectImage(ShadedPathEngine* s)
{
    Log("DirectImage constructor\n");
    setEngine(s);
}

void DirectImage::consume(GPUImage* gpui)
{
    dumpToFile(gpui);
}

DirectImage::~DirectImage()
{
    Log("DirectImage destructor\n");
}

void DirectImage::dumpToFile(GPUImage* gpui)
{
	auto& global = engine->globalRendering;
	auto& device = global.device;

    GPUImage target;
	global.createDumpImage(target);
	auto commandBuffer = global.beginSingleTimeCommands(false);
    dumpToFile(gpui, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);

	global.destroyImage(&target);
	//VkCommandBufferAllocateInfo allocInfo{};
	//allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	//allocInfo.commandPool = tr.commandPool;
	//allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	//allocInfo.commandBufferCount = (uint32_t)1;

	//if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferImageDump) != VK_SUCCESS) {
	//	Error("failed to allocate command buffers!");
	//}
}

void DirectImage::dumpToFile(GPUImage* gpui, VkCommandBuffer commandBuffer)
{
}
