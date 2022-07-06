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
	auto ktxresult = ktxTexture_CreateFromMemory((const ktx_uint8_t*)file_buffer.data(), file_buffer.size(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_CreateFromMemory " << ktxresult);
		Error("Could not create texture from memory");
	}

	VkFormat format = ktxTexture_GetVkFormat(kTexture);
	if (format == VK_FORMAT_UNDEFINED) {
		// for KTX 2 we get an undefined format (currently don't know ig this is a bug or not)
		// in that case we have to set the format before calling VkUpload
		if (kTexture->classId != class_id::ktxTexture2_c) {
			Error("cannot fix texture format for non-KTX 2 textures");
		} else {
			ktxTexture2* t2 = (ktxTexture2*)(kTexture);
			bool needTranscoding = ktxTexture2_NeedsTranscoding(t2);
			if (needTranscoding) {
				// VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
				// VK_FORMAT_BC7_SRGB_BLOCK
				// check phys device support for image format:
				VkFormatProperties fp{}; // output goes here
				fp.linearTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
				//fp.optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
				//fp.bufferFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
				vkGetPhysicalDeviceFormatProperties(engine->global.physicalDevice, VK_FORMAT_BC7_SRGB_BLOCK, &fp);
				ktxresult = ktxTexture2_TranscodeBasis(t2, KTX_TTF_BC7_RGBA, 0);
				if (ktxresult != KTX_SUCCESS) {
					Log("ERROR: in ktxTexture2_TranscodeBasis " << ktxresult);
					Error("Could not uncompress texture");
				}
				needTranscoding = ktxTexture2_NeedsTranscoding(t2);
				assert(needTranscoding == false);
				format = ktxTexture_GetVkFormat(kTexture);
				Log("format: " << format << endl);
				ktxresult = ktxTexture2_VkUploadEx(t2, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				if (ktxresult != KTX_SUCCESS) {
					Log("ERROR: in ktxTexture2_VkUploadEx " << ktxresult);
					Error("Could not upload texture to GPU ktxTexture2_VkUploadEx");
				}
				//ktxTexture2_Destroy(t2);
				// create image view and sampler:
				texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, VK_FORMAT_BC7_SRGB_BLOCK, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
				texture->available = true;
				return;
			}
			//ktxresult = ktxTexture2_DeflateZstd(t2, 0);
			//if (ktxresult != KTX_SUCCESS) {
			//	Log("ERROR: in ktxTexture2_DeflateZstd " << ktxresult);
			//	Error("Could not deflate supercompression");
			//}
			//t2->vkFormat = VK_FORMAT_R8G8B8A8_SRGB;// GlobalRendering::ImageFormat;
		}
	}
	format = ktxTexture_GetVkFormat(kTexture);
	Log("format: " << format << endl);
	ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
		Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
	}

	ktxTexture_Destroy(kTexture);
	// create image view and sampler:
	texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
	texture->available = true;
}

void TextureStore::createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTexAdr)
{
	ktxTexture2* ktxt2;
	//auto ktxresult = ktxTexture2_CreateFromMemory((const ktx_uint8_t*)data, size, KTX_TEXTURE_CREATE_NO_FLAGS, ktxTexAdr);
	auto ktxresult = ktxTexture2_CreateFromMemory((const ktx_uint8_t*)data, size, KTX_TEXTURE_CREATE_NO_FLAGS, &ktxt2);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_CreateFromMemory " << ktxresult);
		Error("Could not create texture from memory");
	}
	TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
	initialTexture.id = "xxx";
	textures["xxx"] = initialTexture;
	TextureInfo* texture = &textures["xxx"];
	texture->vulkanTexture.image = nullptr;
	ktxt2->vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat format = ktxTexture2_GetVkFormat(ktxt2);
	Log("format: " << format << endl);

	//ktxresult = ktxTexture2_VkUploadEx(*ktxTexAdr, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	ktxresult = ktxTexture2_VkUploadEx(ktxt2, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
		engine->files.writeFile("xxx.ktx", (const char*)data, size);
		Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
	}
}

void TextureStore::loadMeshTextures(MeshInfo* mesh)
{
	stringstream idss;
	int texIndex = 0;
	for (auto& texParse : mesh->textureParseInfo) {
		idss << mesh->id << texIndex++;
		// make sure we do not already have this texture stored:
		string id = idss.str();
		if (textures.find(id) != textures.end()) {
			Error("texture already loded");
		}
		TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
		initialTexture.id = id;
		textures[id] = initialTexture;
		TextureInfo* texture = &textures[id];

		auto ktxresult = ktxTexture_VkUploadEx(texParse, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
		}

		ktxTexture_Destroy(texParse);
		// create image view and sampler:
		texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		texture->available = true;
	}
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

