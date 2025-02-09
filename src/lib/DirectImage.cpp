#include "mainheader.h"

using namespace std;

DirectImage::DirectImage(ShadedPathEngine* s)
{
	Log("DirectImage constructor with engine\n");
	setEngine(s);
}

DirectImage::DirectImage()
{
	Log("DirectImage constructor\n");
}

void DirectImage::consume(FrameResources* fi)
{
    dumpToFile(fi->renderedImage);
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
	engine->util.debugNameObjectImage(target.fba.image, "dumpToFile target image");
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
	toLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_2_TRANSFER_WRITE_BIT, commandBuffer, gpui_target);
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
		gpui_source->fba.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		gpui_target->fba.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageCopyRegion);

	toLayout(VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_2_MEMORY_READ_BIT, commandBuffer, gpui_target);

    // revert src image layout and access to before the image data copy
	//toLayout(oldLayout, oldAccess, commandBuffer, gpui_source);
}

void DirectImage::toLayout(VkImageLayout layout, VkAccessFlags2 access, VkCommandBuffer commandBuffer, GPUImage* gpui)
{
	// maybe layout is already correct
    if (layout == gpui->layout) return;

    VkImageMemoryBarrier2 dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	dstBarrier.srcAccessMask = gpui->access;
	dstBarrier.dstAccessMask = access;
	dstBarrier.oldLayout = gpui->layout;
	dstBarrier.newLayout = layout;
	dstBarrier.image = gpui->fba.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    dstBarrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

	VkDependencyInfo dependency_info{};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.imageMemoryBarrierCount = 1;
	dependency_info.pImageMemoryBarriers = &dstBarrier;
	vkCmdPipelineBarrier2(commandBuffer, &dependency_info);
	gpui->layout = layout;
	gpui->access = access;
}

//DirectImage::toLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT| VK_ACCESS_2_MEMORY_WRITE_BIT, winfo->commandBufferPresentBack, &dstImage);
void DirectImage::toLayout(VkImageLayout layout, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkCommandBuffer commandBuffer, GPUImage* gpui)
{
	// maybe layout is already correct
	if (layout == gpui->layout) return;

	VkImageMemoryBarrier2 dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	dstBarrier.srcAccessMask = gpui->access;
	dstBarrier.dstAccessMask = access;
	dstBarrier.oldLayout = gpui->layout;
	dstBarrier.newLayout = layout;
	dstBarrier.image = gpui->fba.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	dstBarrier.srcStageMask = gpui->stage;
	dstBarrier.dstStageMask = stage;

	VkDependencyInfo dependency_info{};
	dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.imageMemoryBarrierCount = 1;
	dependency_info.pImageMemoryBarriers = &dstBarrier;
	vkCmdPipelineBarrier2(commandBuffer, &dependency_info);
	gpui->layout = layout;
	gpui->access = access;
	gpui->stage = stage;
}

void DirectImage::toLayoutAllStagesOnlyForDebugging(VkImageLayout layout, VkCommandBuffer commandBuffer, GPUImage* gpui)
{
	// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#full-pipeline-barrier
	VkImageMemoryBarrier2 dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	dstBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
	dstBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR | VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
	dstBarrier.oldLayout = gpui->layout;
	//dstBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	dstBarrier.image = gpui->fba.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	dstBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;
	dstBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR;

	VkDependencyInfo dependency_info{};
	dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency_info.imageMemoryBarrierCount = 1;
	dependency_info.pImageMemoryBarriers = &dstBarrier;
	vkCmdPipelineBarrier2(commandBuffer, &dependency_info);
	//dstImage.layout = dstBarrier.newLayout;
	//dstImage.access = dstBarrier.dstAccessMask;
}

void DirectImage::openForCPUWriteAccess(GPUImage* gpui, GPUImage* writeable)
{
	assert(writeable != nullptr);
    assert(writeable->fba.image != nullptr);
	assert(gpui != nullptr);
	auto& global = engine->globalRendering;
	auto& device = global.device;

    if (writeable->imagedata == nullptr) {
		Error("DirectImage::openForCPUWriteAccess: writeable image has no imagedata. Did you use GlobalRendering::createDumpImage() to create it?");
    }
	engine->util.debugNameObjectImage(writeable->fba.image, "copy target for write access");
	auto commandBuffer = global.beginSingleTimeCommands(false);
	copyBackbufferImage(gpui, writeable, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);
}

void DirectImage::closeCPUWriteAccess(GPUImage* gpui, GPUImage* writeable)
{
	auto& global = engine->globalRendering;
	auto commandBuffer = global.beginSingleTimeCommands(false);
	copyBackbufferImage(writeable, gpui, commandBuffer);
	global.endSingleTimeCommands(commandBuffer);
}
