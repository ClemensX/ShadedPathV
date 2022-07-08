#include "pch.h"

void TextureStore::init(ShadedPathEngine* engine) {
	this->engine = engine;
	// Set up Vulkan physical device (gpu), logical device (device), queue
	// and command pool. Save the handles to these in a struct called vkctx.
	// ktx VulkanDeviceInfo is used to pass these with the expectation that
	// apps are likely to upload a large number of textures.
	auto ktxresult = ktxVulkanDeviceInfo_Construct(&vdi, engine->global.physicalDevice, engine->global.device, engine->global.graphicsQueue, engine->global.commandPool, nullptr);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxVulkanDeviceInfo_Construct " << ktxresult);
		Error("Could not init ktxVulkanDeviceInfo_Construct");
	}
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
	createKTXFromMemory((const ktx_uint8_t*)file_buffer.data(), file_buffer.size(), &kTexture);
	createVulkanTextureFromKTKTexture(kTexture, texture);
}

void TextureStore::createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTexAdr)
{
	auto ktxresult = ktxTexture_CreateFromMemory((const ktx_uint8_t*)data, size, KTX_TEXTURE_CREATE_NO_FLAGS, ktxTexAdr);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_CreateFromMemory " << ktxresult);
		Error("Could not create texture from memory");
	}

}

void TextureStore::createVulkanTextureFromKTKTexture(ktxTexture* kTexture, TextureInfo* texture)
{
	if (kTexture->classId == class_id::ktxTexture2_c) {
		// for KTX 2 handling
		ktxTexture2* t2 = (ktxTexture2*)(kTexture);
		bool needTranscoding = ktxTexture2_NeedsTranscoding(t2);
		if (needTranscoding) {
			auto ktxresult = ktxTexture2_TranscodeBasis(t2, KTX_TTF_BC7_RGBA, 0);
			if (ktxresult != KTX_SUCCESS) {
				Log("ERROR: in ktxTexture2_TranscodeBasis " << ktxresult);
				Error("Could not uncompress texture");
			}
			needTranscoding = ktxTexture2_NeedsTranscoding(t2);
			assert(needTranscoding == false);
		}
		auto format = ktxTexture_GetVkFormat(kTexture);
		// we should have VK_FORMAT_BC7_UNORM_BLOCK = 145 or VK_FORMAT_BC7_SRGB_BLOCK = 146,
		Log("format: " << format << endl);
		auto ktxresult = ktxTexture2_VkUploadEx(t2, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture2_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture2_VkUploadEx");
		}
		// create image view and sampler:
		texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		texture->available = true;
		return;
	} else {
		// KTX 1 handling
		//auto format = ktxTexture_GetVkFormat(kTexture);
		//Log("format: " << format << endl);
		auto ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
		}
		// create image view and sampler:
		texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		texture->available = true;
	}
	ktxTexture_Destroy(kTexture);
}

TextureInfo* TextureStore::createTextureSlot(MeshInfo* mesh, int index)
{
	stringstream idss;
	assert(index < 10);
	idss << mesh->id << index;
	// make sure we do not already have this texture stored:
	string id = idss.str();
	if (textures.find(id) != textures.end()) {
		Error("texture already loded");
	}
	TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo* texture = &textures[id];
	return texture;
}

void TextureStore::destroyKTXIntermediate(ktxTexture* ktxTex)
{
	ktxTexture_Destroy(ktxTex);
}

TextureStore::~TextureStore()
{
	for (auto& tex : textures) {
		if (tex.second.available) {
			vkDestroyImageView(engine->global.device, tex.second.imageView, nullptr);
			ktxVulkanTexture_Destruct(&tex.second.vulkanTexture, engine->global.device, nullptr);
		}
	}
	ktxVulkanDeviceInfo_Destruct(&vdi);
}

