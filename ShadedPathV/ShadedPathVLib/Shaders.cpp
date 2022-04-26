#include "pch.h"

Shaders::Config& Shaders::Config::init()
{
	engine->global.createViewportState(shaderState);
	// mark last shader
	shaderList[shaderList.size() - 1]->setLastShader(true);
	for (ShaderBase* shader : shaderList) {
		shader->init(*engine, shaderState);
		// pipelines must be created for every rendering thread
		for (auto& res : engine->threadResources) {
			shader->initSingle(res, shaderState);
		}
		shaderState.advance(engine, shader);
	}
	return *this;
}

void Shaders::Config::gatherActiveCommandBuffers(ThreadResources& tr)
{
	tr.activeCommandBuffers.clear();
	for (ShaderBase* shader : shaderList) {
		shader->addCurrentCommandBuffer(tr);
	}
}

VkShaderModule Shaders::createShaderModule(const vector<byte>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(engine.global.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		Error("failed to create shader module!");
	}
	return shaderModule;
}

void Shaders::Config::createCommandBuffers(ThreadResources& tr) {
	for (ShaderBase* shader : shaderList) {
		shader->createCommandBuffer(tr);
	}
}

void Shaders::createCommandBuffers(ThreadResources& tr)
{
	config.createCommandBuffers(tr);
}

void Shaders::checkShaderState(ShadedPathEngine& engine) {
	engine.shaders.config.checkShaderState();
}

void Shaders::gatherActiveCommandBuffers(ThreadResources& tr) {
	engine.shaders.config.gatherActiveCommandBuffers(tr);
}

// SHADER Triangle

// Be aware of local arrays - they will be overwritten after leaving this method!!
// TODO remove clear / push_back cycle
void Shaders::submitFrame(ThreadResources& tr)
{
	//Log("draw index " << engine.currentFrameIndex << endl);

	//simpleShader.updatePerFrame(tr);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//VkSemaphore waitSemaphores[] = { tr.imageAvailableSemaphore };
	//VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//tr.waitStages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;//waitSemaphores;
	//submitInfo.pWaitDstStageMask = &tr.waitStages[0];
	gatherActiveCommandBuffers(tr);
	assert(tr.activeCommandBuffers.size() > 0 && tr.activeCommandBuffers.size() < THREAD_RESOURCES_MAX_COMMAND_BUFFERS);
	for (int i = 0; i < tr.activeCommandBuffers.size(); i++) {
		tr.commandBuffers[i] = tr.activeCommandBuffers[i];
	}
	submitInfo.commandBufferCount = tr.activeCommandBuffers.size();
	submitInfo.pCommandBuffers = &tr.commandBuffers[0];
	//VkSemaphore signalSemaphores[] = { tr.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr; // signalSemaphores;
	tr.submitinfos.clear();
	tr.submitinfos.push_back(submitInfo);
}

void Shaders::initiateShader_BackBufferImageDump()
{
	enabledImageDump = true;
	for (auto& res : engine.threadResources) {
		initiateShader_BackBufferImageDumpSingle(res);
	}
}

// SHADER BackBufferImageDump

void Shaders::initiateShader_BackBufferImageDumpSingle(ThreadResources& res)
{
	enabledImageDump = true;
	auto& device = engine.global.device;
	auto& global = engine.global;
	global.createImage(engine.getBackBufferExtent().width, engine.getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, global.ImageFormat, VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		res.imageDumpAttachment.image, res.imageDumpAttachment.memory);
	// Get layout of the image (including row pitch)
	VkImageSubresource subResource{};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkSubresourceLayout subResourceLayout;

	vkGetImageSubresourceLayout(device, res.imageDumpAttachment.image, &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	vkMapMemory(device, res.imageDumpAttachment.memory, 0, VK_WHOLE_SIZE, 0, (void**)&res.imagedata);
	res.imagedata += subResourceLayout.offset;
	res.subResourceLayout = subResourceLayout;
}

void Shaders::executeBufferImageDump(ThreadResources& tr)
{
	if (!enabledImageDump) return;
	auto& device = engine.global.device;
	auto& global = engine.global;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = tr.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &tr.commandBufferImageDump) != VK_SUCCESS) {
		Error("failed to allocate command buffers!");
	}
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(tr.commandBufferImageDump, &beginInfo) != VK_SUCCESS) {
		Error("failed to begin recording triangle command buffer!");
	}

	// Transition destination image to transfer destination layout
	VkImageMemoryBarrier dstBarrier{};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcAccessMask = 0;
	dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier.image = tr.imageDumpAttachment.image;
	dstBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferImageDump, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier);

	VkImageCopy imageCopyRegion{};
	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.srcSubresource.layerCount = 1;
	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.dstSubresource.layerCount = 1;
	imageCopyRegion.extent.width = engine.getBackBufferExtent().width;
	imageCopyRegion.extent.height = engine.getBackBufferExtent().height;
	imageCopyRegion.extent.depth = 1;

	vkCmdCopyImage(
		tr.commandBufferImageDump,
		tr.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		tr.imageDumpAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageCopyRegion);

	VkImageMemoryBarrier dstBarrier2{};
	dstBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	dstBarrier2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dstBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	dstBarrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	dstBarrier2.image = tr.imageDumpAttachment.image;
	dstBarrier2.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(tr.commandBufferImageDump, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &dstBarrier2);
	if (vkEndCommandBuffer(tr.commandBufferImageDump) != VK_SUCCESS) {
		Error("failed to record triangle command buffer!");
	}
	vkWaitForFences(engine.global.device, 1, &tr.imageDumpFence, VK_TRUE, UINT64_MAX);
	vkResetFences(engine.global.device, 1, &tr.imageDumpFence);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tr.commandBufferImageDump;
	if (vkQueueSubmit(engine.global.graphicsQueue, 1, &submitInfo, tr.imageDumpFence) != VK_SUCCESS) {
		Error("failed to submit draw command buffer!");
	}
	vkDeviceWaitIdle(device);
	vkFreeCommandBuffers(device, tr.commandPool, 1, &tr.commandBufferImageDump);
	// now copy image data to file:
	stringstream name;
	name << "out_" << setw(2) << setfill('0') << imageCouter++ << ".ppm";
	auto filename = engine.files.findFile(name.str(), FileCategory::TEXTURE, false, true);
	if (!engine.files.checkFileForWrite(filename)) {
		return;
	}
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	int32_t height = imageCopyRegion.extent.height;
	int32_t width = imageCopyRegion.extent.width;
	// ppm header
	file << "P6\n" << imageCopyRegion.extent.width << "\n" << imageCopyRegion.extent.height << "\n" << 255 << "\n";

	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	// Check if source is BGR and needs swizzle
	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
	const bool colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());
	const char* imagedata = tr.imagedata;
	// ppm binary pixel data
	for (int32_t y = 0; y < height; y++) {
		unsigned int* row = (unsigned int*)imagedata;
		for (int32_t x = 0; x < width; x++) {
			if (colorSwizzle) {
				file.write((char*)row + 2, 1);
				file.write((char*)row + 1, 1);
				file.write((char*)row, 1);
			}
			else {
				file.write((char*)row, 3);
			}
			row++;
		}
		imagedata += tr.subResourceLayout.rowPitch;
	}
	file.close();
	Log("written image dump file: " << engine.files.absoluteFilePath(filename).c_str() << endl);
}

void Shaders::queueSubmit(ThreadResources& tr)
{
	LogCondF(LOG_QUEUE, "queue submit image index " << tr.frameIndex << endl);
	if (tr.submitinfos.size() == 0) {
		Error("Nothing to submit for frame. Did you forget to call Shaders::submitFrame(ThreadResources& tr)?");
	}
	if (vkQueueSubmit(engine.global.graphicsQueue, 1, &tr.submitinfos.at(0), tr.inFlightFence) != VK_SUCCESS) {
		Error("failed to submit draw command buffer!");
	}
}

Shaders::~Shaders()
{
	Log("Shaders destructor\n");
}