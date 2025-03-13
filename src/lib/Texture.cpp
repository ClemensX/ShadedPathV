#include "mainheader.h"
#include "Texture.h"

using namespace std;

void TextureStore::init(ShadedPathEngine* engine, size_t maxTextures) {
	this->engine = engine;
	this->maxTextures = maxTextures;
	// Set up Vulkan physical device (gpu), logical device (device), queue
	// and command pool. Save the handles to these in a struct called vkctx.
	// ktx VulkanDeviceInfo is used to pass these with the expectation that
	// apps are likely to upload a large number of textures.
	auto ktxresult = ktxVulkanDeviceInfo_Construct(&vdi, engine->globalRendering.physicalDevice, engine->globalRendering.device, engine->globalRendering.graphicsQueue, engine->globalRendering.commandPool, nullptr);
	if (ktxresult != KTX_SUCCESS) {
		Log("ERROR: in ktxVulkanDeviceInfo_Construct " << ktxresult);
		Error("Could not init ktxVulkanDeviceInfo_Construct");
	}
}

TextureInfo* TextureStore::getTextureByIndex(uint32_t index)
{
	auto& allTex = getTexturesMap();
	for (auto& tex : allTex) {
		auto& ti = tex.second;
		if (ti.isAvailable()) {
            if (ti.index == index) {
                return getTexture(ti.id);
            }
		}
	}
    Error("Texture not found by index");
    return nullptr; // keep compiler happy
}

TextureInfo* TextureStore::getTexture(string id)
{
	TextureInfo* ret = &textures[id];
	// simple validity check for now:
	if (ret->id.size() > 0) {
		// if there is no id the texture could not be loaded (wrong filename?)
		if (!ret->isAvailable()) {
			Error("Requested texture not available");
		}
		//ret->available = true;
	}
	else {
		Error("Requested texture not available");
	}
	return ret;
}

void TextureStore::loadTexture(string filename, string id, TextureType type, TextureFlags flags)
{
	vector<byte> file_buffer;
	TextureInfo *texture = createTextureSlot(id);
	texture->type = type;

	// find texture file, look in pak file first:
	PakEntry *pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	//initialTexture.filename = filename; // TODO check: field not needed? only in this method? --> remove
	if (pakFileEntry == nullptr) {
		string binFile = engine->files.findFile(filename.c_str(), FileCategory::TEXTURE);
		texture->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(texture->filename.c_str(), file_buffer, FileCategory::TEXTURE);
	} else {
		engine->files.readFile(pakFileEntry, file_buffer, FileCategory::TEXTURE);
	}

	ktxTexture* kTexture;
	createKTXFromMemory((const ktx_uint8_t*)file_buffer.data(), static_cast<int>(file_buffer.size()), &kTexture);
	createVulkanTextureFromKTKTexture(kTexture, texture);
	if (hasFlag(flags, TextureFlags::KEEP_DATA_BUFFER)) {
		assert(kTexture->numLevels == 1);
		assert(texture->vulkanTexture.imageFormat == VK_FORMAT_R32_SFLOAT);
		// store raw buffer for later use
		int level = 0; int layer = 0; int faceSlice = 0; ktx_size_t offset; KTX_error_code result;
		result = ktxTexture_GetImageOffset(kTexture, level, layer, faceSlice, &offset);
		void* data = ktxTexture_GetData(kTexture) + offset;
		auto size = ktxTexture_GetImageSize(kTexture, level);
		assert(size == texture->vulkanTexture.width * texture->vulkanTexture.height * sizeof(float));
		//texture->raw_buffer.insert(texture->raw_buffer.end(), (std::byte*)data, (std::byte*)data + size);
		Log("size: " << size << endl);
		float* floatData = (float*)data;
		texture->float_buffer.insert(texture->float_buffer.end(), floatData, floatData + (size / sizeof(float)));
		Log("size float: " << texture->float_buffer.size() << endl);
		//for (int i = 0; i < size / 4; i++) {
		//	Log("floatData: " << floatData[i] << endl);
		//}
        texture->flags = flags;
	}
	ktxTexture_Destroy(kTexture);
}

void TextureStore::createKTXFromMemory(const unsigned char* data, int size, ktxTexture** ktxTexAdr)
{
	auto ktxresult = ktxTexture_CreateFromMemory((const ktx_uint8_t*)data, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, ktxTexAdr);
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
		auto ktxresult = ktxTexture2_VkUploadEx(t2, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture2_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture2_VkUploadEx");
		}
		if (texture->type == TextureType::TEXTURE_TYPE_MIPMAP_IMAGE && texture->vulkanTexture.levelCount < 2) {
			stringstream s;
			s << "Cannot load TEXTURE_TYPE_MIPMAP_IMAGE texture without mipmaps " << texture->filename << endl;
			Error(s.str());
		} else if (texture->type == TextureType::TEXTURE_TYPE_HEIGHT && texture->vulkanTexture.levelCount > 1) {
			stringstream s;
			s << "Cannot load TEXTURE_TYPE_HEIGHT texture with mipmaps " << texture->filename << endl;
			Error(s.str());
		}
		// create image view and sampler:
		if (kTexture->isCubemap) {
			texture->imageView = engine->globalRendering.createImageViewCube(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		} else {
			texture->imageView = engine->globalRendering.createImageView(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		}
        setTextureActive(texture->id, true);
		return;
	} else {
		// KTX 1 handling
		auto format = ktxTexture_GetVkFormat(kTexture);
		Log("format: " << format << endl);
		auto ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture_VkUploadEx");
		}
		// create image view and sampler:
		if (kTexture->isCubemap) {
			texture->imageView = engine->globalRendering.createImageViewCube(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		} else {
			//texture->imageView = engine->globalRendering.createImageView(texture->vulkanTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
			texture->imageView = engine->globalRendering.createImageView(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		}
		setTextureActive(texture->id, true);
	}
}

TextureInfo* TextureStore::createTextureSlot(string textureName)
{
	// make sure we do not already have this texture stored:
	if (textures.find(textureName) != textures.end()) {
		Error("texture already loaded");
	}
	return internalCreateTextureSlot(textureName);
}

TextureInfo* TextureStore::createTextureSlotForMesh(MeshInfo* mesh, int index)
{
	stringstream idss;
	assert(index < maxTextures);
	idss << mesh->id << index;
	// make sure we do not already have this texture stored:
	string id = idss.str();
	if (textures.find(id) != textures.end()) {
		Error("texture already loded");
	}
	return internalCreateTextureSlot(id);
}

TextureInfo* TextureStore::internalCreateTextureSlot(string id)
{
	TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo* texture = &textures[id];
	checkStoreSize();
	texture->index = static_cast<uint32_t>(textures.size()-1);
	return texture;
}

// brdflut, Irradiance and PrefilteredEnv generation taken from:
// https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/src/main.cpp


void TextureStore::generateCubemaps(std::string skyboxTexture, int32_t dimIrradiance, VkFormat formatIrradiance, int32_t dimPrefilteredEnv, VkFormat formatPrefilteredEnv)
{
	auto& global = engine->globalRendering;
	auto& device = engine->globalRendering.device;
	auto& texStore = engine->textureStore;

	enum Target { IRRADIANCE = 0, PREFILTEREDENV = 1 };

	for (uint32_t target = 0; target < PREFILTEREDENV + 1; target++) {
		VkFormat format;
		int32_t dim;
        TextureInfo* cubemap = nullptr;
		VkSampler cubemapSampler = nullptr;

		switch (target) {
		case IRRADIANCE:
            format = formatIrradiance;
			dim = dimIrradiance;
            cubemap = texStore.createTextureSlot(IRRADIANCE_TEXTURE_ID);
			break;
		case PREFILTEREDENV:
            format = formatPrefilteredEnv;
			dim = dimPrefilteredEnv;
			cubemap = texStore.createTextureSlot(PREFILTEREDENV_TEXTURE_ID);
			break;
		};

		const uint32_t numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
		// Create target cubemap
		{
			FrameBufferAttachment attachment{};

			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			//VkImageTiling tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
			global.createImageCube(dim, dim, numMips, VK_SAMPLE_COUNT_1_BIT, format,
				tiling, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | /*VK_IMAGE_USAGE_TRANSFER_DST_BIT | */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, attachment.image, attachment.memory, cubemap->id.c_str());

			// View
			cubemap->vulkanTexture.deviceMemory = nullptr;
			cubemap->vulkanTexture.layerCount = 6;
			cubemap->vulkanTexture.imageFormat = format;
			cubemap->vulkanTexture.image = attachment.image;
			cubemap->vulkanTexture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			cubemap->vulkanTexture.deviceMemory = attachment.memory;
			cubemap->vulkanTexture.depth = 1; // TODO check
			//cubemap->vulkanTexture.imageLayout = 0; // TODO check
			cubemap->vulkanTexture.height = dim;
			cubemap->vulkanTexture.width = dim;
			cubemap->vulkanTexture.levelCount = numMips;
			cubemap->isKtxCreated = false;
			cubemap->imageView = global.createImageView(cubemap->vulkanTexture.image, cubemap->vulkanTexture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			// Sampler
			VkSamplerCreateInfo samplerCI{};
			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.magFilter = VK_FILTER_LINEAR;
			samplerCI.minFilter = VK_FILTER_LINEAR;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = static_cast<float>(numMips);
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			cubemapSampler = engine->globalRendering.samplerCache.getOrCreateSampler(engine->globalRendering.device, samplerCI);
            //Log("Created cubemap sampler: " << hex << (void*)cubemapSampler << endl);
		}
		// FB, Att, RP, Pipe, etc.
		VkAttachmentDescription attDesc{};
		// Color attachment
		attDesc.format = format;
		attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Renderpass
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = 1;
		renderPassCI.pAttachments = &attDesc;
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDescription;
		renderPassCI.dependencyCount = 2;
		renderPassCI.pDependencies = dependencies.data();
		VkRenderPass renderpass;
        if (vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass) != VK_SUCCESS) {
            Error("Cannot create render pass in cubemap generation");
        }

		struct Offscreen {
			VkImage image;
			VkImageView view;
			VkDeviceMemory memory;
			VkFramebuffer framebuffer;
		} offscreen;

		// Create offscreen framebuffer
		{
			// Image
			VkImageCreateInfo imageCI{};
			imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = format;
			imageCI.extent.width = dim;
			imageCI.extent.height = dim;
			imageCI.extent.depth = 1;
			imageCI.mipLevels = 1;
			imageCI.arrayLayers = 1;
			imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; // do not use due to unable to write image
			//imageCI.tiling = VK_IMAGE_TILING_LINEAR;
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateImage(device, &imageCI, nullptr, &offscreen.image) != VK_SUCCESS) {
                Error("Cannot create offscreen image in cubemap generation");
            }
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, offscreen.image, &memReqs);
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = global.findMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(device, &memAllocInfo, nullptr, &offscreen.memory) != VK_SUCCESS) {
                Error("Cannot allocate offscreen image memory in cubemap generation");
            }
			if (vkBindImageMemory(device, offscreen.image, offscreen.memory, 0) != VK_SUCCESS) {
                Error("Cannot bind offscreen image memory in cubemap generation");
            }

			// View
			VkImageViewCreateInfo viewCI{};
			viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCI.format = format;
			viewCI.flags = 0;
			viewCI.subresourceRange = {};
			viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCI.subresourceRange.baseMipLevel = 0;
			viewCI.subresourceRange.levelCount = 1;
			viewCI.subresourceRange.baseArrayLayer = 0;
			viewCI.subresourceRange.layerCount = 1;
			viewCI.image = offscreen.image;
            if (vkCreateImageView(device, &viewCI, nullptr, &offscreen.view) != VK_SUCCESS) {
                Error("Cannot create offscreen image view in cubemap generation");
            }

			// Framebuffer
			VkFramebufferCreateInfo framebufferCI{};
			framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCI.renderPass = renderpass;
			framebufferCI.attachmentCount = 1;
			framebufferCI.pAttachments = &offscreen.view;
			framebufferCI.width = dim;
			framebufferCI.height = dim;
			framebufferCI.layers = 1;
            if (vkCreateFramebuffer(device, &framebufferCI, nullptr, &offscreen.framebuffer) != VK_SUCCESS) {
                Error("Cannot create offscreen framebuffer in cubemap generation");
            }

			VkCommandBuffer layoutCmd = global.beginSingleTimeCommands();
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.image = offscreen.image;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdPipelineBarrier(layoutCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			global.endSingleTimeCommands(layoutCmd, true);
		}

        // Descriptors
		VkDescriptorSetLayout descriptorsetlayout;
		//VkDescriptorSetLayoutBinding setLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
		descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		//descriptorSetLayoutCI.pBindings = &setLayoutBinding;
		//descriptorSetLayoutCI.bindingCount = 1;
        if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorsetlayout) != VK_SUCCESS) {
            Error("Cannot create descriptor set layout in cubemap generation");
        }

		// Descriptor Pool
		VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = 1;
		descriptorPoolCI.pPoolSizes = &poolSize;
		descriptorPoolCI.maxSets = 2;
		VkDescriptorPool descriptorpool;
        if (vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorpool) != VK_SUCCESS) {
            Error("Cannot create descriptor pool in cubemap generation");
        }

		// Descriptor sets
		VkDescriptorSet descriptorset;
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorpool;
		descriptorSetAllocInfo.pSetLayouts = &descriptorsetlayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
        if (vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorset) != VK_SUCCESS) {
            Error("Cannot allocate descriptor set in cubemap generation");
        }
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = descriptorset;
		writeDescriptorSet.dstBinding = 0;
        TextureInfo* environmentCube = engine->textureStore.getTexture(skyboxTexture); // part of global texture array
		// we don't write desc set for image - use from global array instead
/**/		//writeDescriptorSet.pImageInfo = &environmentCube->vulkanTexture..descriptor;
		//vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

		struct PushBlockIrradiance {
			glm::mat4 mvp;
			uint32_t textureIndex;
			float deltaPhi = (2.0f * float(PI)) / 180.0f;
			float deltaTheta = (0.5f * float(PI)) / 64.0f;
		} pushBlockIrradiance;
        pushBlockIrradiance.textureIndex = environmentCube->index;
		struct PushBlockPrefilterEnv {
			glm::mat4 mvp;
			uint32_t textureIndex;
			float roughness;
			uint32_t numSamples = 32u;
		} pushBlockPrefilterEnv;
        pushBlockPrefilterEnv.textureIndex = environmentCube->index;

		// Pipeline layout
		VkPipelineLayout pipelinelayout;
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		switch (target) {
		case IRRADIANCE:
			pushConstantRange.size = sizeof(PushBlockIrradiance);
			break;
		case PREFILTEREDENV:
			pushConstantRange.size = sizeof(PushBlockPrefilterEnv);
			break;
		};

		vector<VkDescriptorSetLayout> sets;
		sets.push_back(descriptorsetlayout);
		if (engine->textureStore.layout) {
			sets.push_back(engine->textureStore.layout);
		}
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		//pipelineLayoutCI.setLayoutCount = 1;
		//pipelineLayoutCI.pSetLayouts = &descriptorsetlayout;
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(sets.size());
		pipelineLayoutCI.pSetLayouts = &sets[0];
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout) != VK_SUCCESS) {
            Error("Cannot create pipeline layout in cubemap generation");
        }

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.front = depthStencilStateCI.back;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Vertex input state
		VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(vkglTF_Model_Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		VkVertexInputAttributeDescription vertexInputAttribute = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		//vertexInputStateCI.vertexBindingDescriptionCount = 0;
		//vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		//vertexInputStateCI.vertexAttributeDescriptionCount = 0;
		//vertexInputStateCI.pVertexAttributeDescriptions = &vertexInputAttribute;

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelinelayout;
		pipelineCI.renderPass = renderpass;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.renderPass = renderpass;

		// load shader binary code
		vector<byte> file_buffer_vert;
		vector<byte> file_buffer_frag;
		engine->files.readFile("filtercube.vert.spv", file_buffer_vert, FileCategory::FX);
		//shaderStages[0] = loadShader(device, "filtercube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		switch (target) {
		case IRRADIANCE:
			//shaderStages[1] = loadShader(device, "irradiancecube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			engine->files.readFile("irradiancecube.frag.spv", file_buffer_frag, FileCategory::FX);
			break;
		case PREFILTEREDENV:
			//shaderStages[1] = loadShader(device, "prefilterenvmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			engine->files.readFile("prefilterenvmap.frag.spv", file_buffer_frag, FileCategory::FX);
			break;
		};
		auto vertShaderModule = engine->shaders.createShaderModule(file_buffer_vert);
		auto fragShaderModule = engine->shaders.createShaderModule(file_buffer_frag);
		// create shader stage
		auto vertShaderModuleInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
		auto fragShaderModuleInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
		// Look-up-table (from BRDF) pipeline		
		shaderStages = { vertShaderModuleInfo, fragShaderModuleInfo };
		VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS) {
            Error("Cannot create graphics pipeline in cubemap generation");
        }
		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}

		// Render cubemap
		VkClearValue clearValues[1];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderpass;
		renderPassBeginInfo.framebuffer = offscreen.framebuffer;
		renderPassBeginInfo.renderArea.extent.width = dim;
		renderPassBeginInfo.renderArea.extent.height = dim;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;

		std::vector<glm::mat4> matrices = {
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};


		VkViewport viewport{};
		viewport.width = (float)dim;
		viewport.height = (float)dim;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = dim;
		scissor.extent.height = dim;

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = numMips;
		subresourceRange.layerCount = 6;

		VkCommandBuffer cmdBuf;
		// Change image layout for all cubemap faces to transfer destination
		{
			//vulkanDevice->beginCommandBuffer(cmdBuf);
			cmdBuf = global.beginSingleTimeCommands();
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.image = cubemap->vulkanTexture.image;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//vulkanDevice->flushCommandBuffer(cmdBuf, queue, false);
			global.endSingleTimeCommands(cmdBuf, true);
		}

		for (uint32_t m = 0; m < numMips; m++) {
			for (uint32_t f = 0; f < 6; f++) {

				//vulkanDevice->beginCommandBuffer(cmdBuf);
				cmdBuf = global.beginSingleTimeCommands();

				viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
				viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
				vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
				vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

				// Render scene from cube face's point of view
				vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Pass parameters for current pass using a push constant block
				switch (target) {
				case IRRADIANCE:
					pushBlockIrradiance.mvp = glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
					vkCmdPushConstants(cmdBuf, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockIrradiance), &pushBlockIrradiance);
					break;
				case PREFILTEREDENV:
					pushBlockPrefilterEnv.mvp = glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
					pushBlockPrefilterEnv.roughness = (float)m / (float)(numMips - 1);
					vkCmdPushConstants(cmdBuf, pipelinelayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockPrefilterEnv), &pushBlockPrefilterEnv);
					break;
				};

				vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				//vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL); adapt for global array
				// bind global texture array:
				vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 1, 1, &engine->textureStore.descriptorSet, 0, nullptr);

				VkDeviceSize offsets[1] = { 0 };

//				models.skybox.draw(cmdBuf);
				//vkCmdDrawIndexed(cmdBuf, 36, 1, 0, 0, 0);
				vkCmdDraw(cmdBuf, 36, 1, 0, 0);

				vkCmdEndRenderPass(cmdBuf);

				VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = numMips;
				subresourceRange.layerCount = 6;

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.image = offscreen.image;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
					vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}

				// Copy region for transfer from framebuffer to cube face
				VkImageCopy copyRegion{};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = f;
				copyRegion.dstSubresource.mipLevel = m;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
				copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
				copyRegion.extent.depth = 1;

				vkCmdCopyImage(
					cmdBuf,
					offscreen.image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					cubemap->vulkanTexture.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.image = offscreen.image;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
					vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}

//				vulkanDevice->flushCommandBuffer(cmdBuf, queue, false);
				global.endSingleTimeCommands(cmdBuf, true);
			}
		}

		{
			//vulkanDevice->beginCommandBuffer(cmdBuf);
			cmdBuf = global.beginSingleTimeCommands();
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.image = cubemap->vulkanTexture.image;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//vulkanDevice->flushCommandBuffer(cmdBuf, queue, false);
			global.endSingleTimeCommands(cmdBuf, true);
		}
		cubemap->sampler = cubemapSampler;
		cubemap->vulkanTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		cubemap->type = TextureType::TEXTURE_TYPE_GLTF; // uses the sampler from above
		setTextureActive(cubemap->id, true);
		string filepath;
		switch (target) {
		case IRRADIANCE:
			filepath = engine->files.findFileForCreation("irradiance.ktx2", FileCategory::TEXTURE);
            //Log("Writing irradiance to " << filepath << endl);
			global.writeCubemapToFile(cubemap, filepath);
			break;
		case PREFILTEREDENV:
			filepath = engine->files.findFileForCreation("prefilter.ktx2", FileCategory::TEXTURE);
			global.writeCubemapToFile(cubemap, filepath);
			break;
		}

		// cleanup
        //vkDestroySampler(device, cubemapSampler, nullptr); // destroyed in sampler cache
		vkDestroyRenderPass(device, renderpass, nullptr);
		vkDestroyFramebuffer(device, offscreen.framebuffer, nullptr);
		vkFreeMemory(device, offscreen.memory, nullptr);
		vkDestroyImageView(device, offscreen.view, nullptr);
		vkDestroyImage(device, offscreen.image, nullptr);
		vkDestroyDescriptorPool(device, descriptorpool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
	}
}

void TextureStore::generateBRDFLUT()
{
	auto& global = engine->globalRendering;
	auto& device = engine->globalRendering.device;
	auto& texStore = engine->textureStore;
	const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
	const int32_t dim = 512;

	FrameBufferAttachment attachment{};
	// create entry in texture store:
	auto ti = texStore.createTextureSlot(BRDFLUT_TEXTURE_ID);

	//createImageCube(twoD->vulkanTexture.width, twoD->vulkanTexture.height, twoD->vulkanTexture.levelCount, VK_SAMPLE_COUNT_1_BIT, twoD->vulkanTexture.imageFormat, VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	attachment.image, attachment.memory);
	// create image and imageview:
	global.createImage(dim, dim, 1, VK_SAMPLE_COUNT_1_BIT, format,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | /*VK_IMAGE_USAGE_TRANSFER_DST_BIT | */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		0, attachment.image, attachment.memory, "BRDFLUT");

	ti->vulkanTexture.deviceMemory = nullptr;
	ti->vulkanTexture.layerCount = 1;
	ti->vulkanTexture.imageFormat = format;
	ti->vulkanTexture.image = attachment.image;
	ti->vulkanTexture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	ti->vulkanTexture.deviceMemory = attachment.memory;
	ti->vulkanTexture.depth = 1; // TODO check
	//cubemap->vulkanTexture.imageLayout = 0; // TODO check
	ti->vulkanTexture.height = dim;
	ti->vulkanTexture.width = dim;
	ti->vulkanTexture.levelCount = 1;
	ti->isKtxCreated = false;
	ti->imageView = global.createImageView(ti->vulkanTexture.image, ti->vulkanTexture.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	// Sampler
	VkSamplerCreateInfo samplerCI{};
	samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 1.0f;
	samplerCI.maxAnisotropy = 1.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VkSampler brdfSampler = nullptr;

	brdfSampler = engine->globalRendering.samplerCache.getOrCreateSampler(engine->globalRendering.device, samplerCI);
    ti->sampler = brdfSampler;
    ti->type = TextureType::TEXTURE_TYPE_GLTF; // uses the sampler from above

	// FB, Att, RP, Pipe, etc.
	VkAttachmentDescription attDesc{};
	// Color attachment
	attDesc.format = format;
	attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassCI{};
	renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCI.attachmentCount = 1;
	renderPassCI.pAttachments = &attDesc;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();

	VkRenderPass renderpass;
	if (vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass) != VK_SUCCESS) {
		Error("Cannot create render pass in BRDFLUT generation");
	}

	VkFramebufferCreateInfo framebufferCI{};
	framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCI.renderPass = renderpass;
	framebufferCI.attachmentCount = 1;
	framebufferCI.pAttachments = &ti->imageView;
	framebufferCI.width = dim;
	framebufferCI.height = dim;
	framebufferCI.layers = 1;

	VkFramebuffer framebuffer;
	if (vkCreateFramebuffer(device, &framebufferCI, nullptr, &framebuffer) != VK_SUCCESS) {
		Error("Cannot create framebuffer in BRDFLUT generation");
	}

	// Desriptors
	VkDescriptorSetLayout descriptorsetlayout;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorsetlayout) != VK_SUCCESS) {
		Error("Cannot create descriptor layout in BRDFLUT generation");
	}

	// Pipeline layout
	VkPipelineLayout pipelinelayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &descriptorsetlayout;
	if (vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout) != VK_SUCCESS) {
		Error("Cannot create pipeline layout in BRDFLUT generation");
	}

	// Pipeline
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
	inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
	rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCI.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
	colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCI.attachmentCount = 1;
	colorBlendStateCI.pAttachments = &blendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_FALSE;
	depthStencilStateCI.depthWriteEnable = VK_FALSE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCI.front = depthStencilStateCI.back;
	depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

	VkPipelineViewportStateCreateInfo viewportStateCI{};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;

	VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
	multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI{};
	dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
	dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

	VkPipelineVertexInputStateCreateInfo emptyInputStateCI{};
	emptyInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.layout = pipelinelayout;
	pipelineCI.renderPass = renderpass;
	pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	pipelineCI.pVertexInputState = &emptyInputStateCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = shaderStages.data();

	// load shader binary code
	vector<byte> file_buffer_vert;
	vector<byte> file_buffer_frag;
	engine->files.readFile("genbrdflut.vert.spv", file_buffer_vert, FileCategory::FX);
	engine->files.readFile("genbrdflut.frag.spv", file_buffer_frag, FileCategory::FX);
	//Log("read vertex shader: " << file_buffer_vert.size() << endl);
	//Log("read fragment shader: " << file_buffer_frag.size() << endl);
	// create shader modules
	auto vertShaderModule = engine->shaders.createShaderModule(file_buffer_vert);
	auto fragShaderModule = engine->shaders.createShaderModule(file_buffer_frag);
	// create shader stage
	auto vertShaderModuleInfo = engine->shaders.createVertexShaderCreateInfo(vertShaderModule);
	auto fragShaderModuleInfo = engine->shaders.createFragmentShaderCreateInfo(fragShaderModule);
	// Look-up-table (from BRDF) pipeline		
	shaderStages = { vertShaderModuleInfo, fragShaderModuleInfo };
	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS) {
		Error("Cannot create pipeline in BRDFLUT generation");
	}
	for (auto shaderStage : shaderStages) {
		vkDestroyShaderModule(device, shaderStage.module, nullptr);
	}

	// Render
	VkClearValue clearValues[1];
	clearValues[0].color = { { 1.0f, 0.0f, 0.0f, 1.0f } }; // red

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderpass;
	renderPassBeginInfo.renderArea.extent.width = dim;
	renderPassBeginInfo.renderArea.extent.height = dim;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = framebuffer;

	auto cmdBuf = global.beginSingleTimeCommands();
	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	if (true) {


		VkViewport viewport{};
		viewport.width = (float)dim;
		viewport.height = (float)dim;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = dim;
		scissor.extent.height = dim;

		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(cmdBuf, 3, 1, 0, 0);
	}
	vkCmdEndRenderPass(cmdBuf);
	global.endSingleTimeCommands(cmdBuf, true);

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
	vkDestroyRenderPass(device, renderpass, nullptr);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);
    //vkDestroySampler(device, brdfSampler, nullptr); // destroyed in sampler cache

    setTextureActive(ti->id, true);
}

void TextureStore::destroyKTXIntermediate(ktxTexture* ktxTex)
{
	ktxTexture_Destroy(ktxTex);
}

void TextureStore::checkStoreSize()
{
	if (textures.size() > maxTextures) {
		stringstream s;
		s << "Maximum texture count exceeded: " << maxTextures << endl;
		Error(s.str());
	}
}

void TextureStore::setTextureActive(std::string id, bool active)
{
    auto ti = textures.find(id);
    if (ti != textures.end()) {
        ti->second.available = active;
		// recreate texture pool descriptor set
		VulkanResources::updateDescriptorSetForTextures(engine);
		Log("tex added and descriptor set updated: " << ti->second.id.c_str() << " index: " << ti->second.index << endl);
		return;
	}
    Error("Texture not found");
}

TextureStore::~TextureStore()
{
	auto& device = engine->globalRendering.device;
	for (auto& tex : textures) {
		auto &ti = tex.second;
		Log("Texture found: " << ti.id.c_str() << " " << ti.filename.c_str() << " " << ti.vulkanTexture.deviceMemory << endl);
		if (ti.isAvailable()) {
			vkDestroyImageView(engine->globalRendering.device, tex.second.imageView, nullptr);
			if (ti.isKtxCreated) {
				ktxVulkanTexture_Destruct(&ti.vulkanTexture, engine->globalRendering.device, nullptr);
			} else {
				vkDestroyImage(device, ti.vulkanTexture.image, nullptr);
				vkFreeMemory(device, ti.vulkanTexture.deviceMemory, nullptr);
			}
		}
	}
    if (vdi.device == nullptr) {
        Error("Texture store not properly initialized");
    }
	ktxVulkanDeviceInfo_Destruct(&vdi);
	vkDestroyDescriptorSetLayout(device, layout, nullptr);
	vkDestroyDescriptorPool(device, pool, nullptr);

}

