#include "pch.h"

void TextureStore::init(ShadedPathEngine* engine) {
	this->engine = engine;
}

TextureInfo* TextureStore::getTexture(string id)
{
	TextureInfo *ret = &textures[id];
	// simple validity check for now:
	if (ret->id.size() > 0) {
		// if there is no id the texture could not be loaded (wrong filename?)
		ret->available = true;
	}
	else {
		ret->available = false;
	}
	return ret;
}

void TextureStore::loadTexture(string filename, string id)
{
	TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
	vector<byte> file_buffer;

	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo *texture = &textures[id];

	// find texture file, look in pak file first:
	PakEntry *pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	initialTexture.filename = filename; // TODO check: field not needed? only in this method? --> remove
	if (pakFileEntry == nullptr) {
		string binFile = engine->files.findFile(filename.c_str(), FileCategory::TEXTURE);
		texture->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(texture->filename.c_str(), file_buffer, FileCategory::TEXTURE);
	} else {
		engine->files.readFile(pakFileEntry, file_buffer, FileCategory::TEXTURE);
	}

	ktxTexture* kTexture;
	//KTX_error_code result;
	ktx_size_t offset;
	ktx_uint8_t* image;
	ktx_uint32_t level, layer, faceSlice;
	ktxVulkanDeviceInfo vdi;
	auto ktxresult = ktxTexture_CreateFromMemory((const ktx_uint8_t*)file_buffer.data(), file_buffer.size(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_CreateFromMemory " << ktxresult);
		Error("Could not create texture from memory");
	}

	// Set up Vulkan physical device (gpu), logical device (device), queue
    // and command pool. Save the handles to these in a struct called vkctx.
    // ktx VulkanDeviceInfo is used to pass these with the expectation that
    // apps are likely to upload a large number of textures.
	ktxresult = ktxVulkanDeviceInfo_Construct(&vdi, engine->global.physicalDevice, engine->global.device, engine->global.graphicsQueue, engine->global.commandPool, nullptr);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxVulkanDeviceInfo_Construct " << ktxresult);
		Error("Could not init ktxVulkanDeviceInfo_Construct");
	}
	ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
		Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
	}

	ktxTexture_Destroy(kTexture);
	ktxVulkanDeviceInfo_Destruct(&vdi);

	// create image view and sampler:
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture->vulkanTexture.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(engine->global.device, &viewInfo, nullptr, &texture->imageView) != VK_SUCCESS) {
		Error("failed to create texture image view!");
	}
	texture->available = true;
}

TextureStore::~TextureStore()
{
	for (auto& tex : textures) {
		if (tex.second.available) {
			vkDestroyImageView(engine->global.device, tex.second.imageView, nullptr);
			ktxVulkanTexture_Destruct(&tex.second.vulkanTexture, engine->global.device, nullptr);
		}
	}

}

