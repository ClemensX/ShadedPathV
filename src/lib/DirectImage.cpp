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
	engine->util.debugNameObjectImage(target.image, "dumpToFile target image");
	//engine->util.debugNameObjectImage(gpui->image, "dumptToFile source image");
	auto commandBuffer = global.beginSingleTimeCommands(false);
    copyBackbufferImage(gpui, &target, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);

	// now copy image data to file:
	stringstream name;
	name << "out_" << setw(2) << setfill('0') << imageCounter++ << ".ppm";
	//auto filename = engine.files.findFile(name.str(), FileCategory::TEXTURE, false, true); // we rather use current directory, maybe reconsider later
	auto filename = name.str();
	//if (!engine->files.checkFileForWrite(filename)) {
	//	Log("Could not write image dump file: " << filename << endl);
	//	return;
	//}
	// If source is BGR (destination is always RGB), we'll have to manually swizzle color components
	// Crazy way to check if source is BGR and dest is RGB. As we already know that we could also simply set colorSwizzle to true...
	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
	const bool colorSwizzle = !(std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());
	engine->util.writePPM(filename, target.imagedata, engine->getBackBufferExtent().width, engine->getBackBufferExtent().height,
		target.subResourceLayout.rowPitch, colorSwizzle);

	global.destroyImage(&target);
}

void DirectImage::copyBackbufferImage(GPUImage* gpui_source, GPUImage* gpui_target, VkCommandBuffer commandBuffer)
{
	// Transition destination image to transfer destination layout
    //auto oldLayout = gpui_source->layout;
    //auto oldAccess = gpui_source->access;
	toLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, commandBuffer, gpui_target);
	toLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_2_TRANSFER_READ_BIT, commandBuffer, gpui_source);

	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.layerCount = 1;
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.layerCount = 1;
	imageCopyRegion.extent.width = engine->getBackBufferExtent().width;
	imageCopyRegion.extent.height = engine->getBackBufferExtent().height;
	imageCopyRegion.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer,
		gpui_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		gpui_target->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageCopyRegion);

	toLayout(VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_MEMORY_READ_BIT, commandBuffer, gpui_target);

    // revert src image layout and access to before the image data copy
	//toLayout(oldLayout, oldAccess, commandBuffer, gpui_source);
}

void DirectImage::toLayout(VkImageLayout layout, VkAccessFlags access, VkCommandBuffer commandBuffer, GPUImage* gpui)
{
	// maybe layout is already correct
    if (layout == gpui->layout) return;

	VkImageMemoryBarrier dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcAccessMask = gpui->access;
	dstBarrier.dstAccessMask = access;
	dstBarrier.oldLayout = gpui->layout;
	dstBarrier.newLayout = layout;
	dstBarrier.image = gpui->image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier);
    gpui->layout = layout;
    gpui->access = access;
}

void DirectImage::openForCPUWriteAccess(GPUImage* gpui, GPUImage* writeable)
{
	assert(writeable != nullptr);
	assert(gpui != nullptr);
	auto& global = engine->globalRendering;
	auto& device = global.device;

	global.createDumpImage(*writeable);
	engine->util.debugNameObjectImage(writeable->image, "copy target for write access");
	//engine->util.debugNameObjectImage(gpui->image, "dumptToFile source image");
	auto commandBuffer = global.beginSingleTimeCommands(false);
	copyBackbufferImage(gpui, writeable, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);
	for (int i = 0; i < 100000; i++) {
		unsigned int* row = (unsigned int*)(writeable->imagedata + i);
		*((char*)row) = 0xff;
	}
}

void DirectImage::closeCPUWriteAccess(GPUImage* gpui, GPUImage* writeable)
{
	auto& global = engine->globalRendering;
	auto commandBuffer = global.beginSingleTimeCommands(false);
	copyBackbufferImage(writeable, gpui, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);
	global.destroyImage(writeable);
}
