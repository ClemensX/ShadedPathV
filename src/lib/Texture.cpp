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
		Error("Requested texture not available");
		ret->available = false;
	}
	return ret;
}

void TextureStore::loadTexture(string filename, string id)
{
	vector<byte> file_buffer;
	TextureInfo *texture = createTextureSlot(id);

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
		auto ktxresult = ktxTexture2_VkUploadEx(t2, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (ktxresult != KTX_SUCCESS) {
			Log("ERROR: in ktxTexture2_VkUploadEx " << ktxresult);
			Error("Could not upload texture to GPU ktxTexture2_VkUploadEx");
		}
		if (texture->vulkanTexture.levelCount < 2) {
			Error("Cannot load texture without mipmaps");
		}
		// create image view and sampler:
		if (kTexture->isCubemap) {
			texture->imageView = engine->global.createImageViewCube(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		} else {
			texture->imageView = engine->global.createImageView(texture->vulkanTexture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, texture->vulkanTexture.levelCount);
		}
		texture->available = true;
		return;
	} else {
		// KTX 1 handling
		//auto format = ktxTexture_GetVkFormat(kTexture);
		//Log("format: " << format << endl);
		auto ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture->vulkanTexture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

void TextureStore::generateBRDFLUT()
{
	auto& global = engine->global;
	auto& device = engine->global.device;
	auto& texStore = engine->textureStore;
	const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
	const int32_t dim = 512;

	// create entry in texture store:
	FrameBufferAttachment attachment{};
	auto ti = texStore.createTextureSlot(BRDFLUT_TEXTURE_ID);

	//createImageCube(twoD->vulkanTexture.width, twoD->vulkanTexture.height, twoD->vulkanTexture.levelCount, VK_SAMPLE_COUNT_1_BIT, twoD->vulkanTexture.imageFormat, VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	attachment.image, attachment.memory);
	// create image and imageview:
	global.createImage(dim, dim, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | /*VK_IMAGE_USAGE_TRANSFER_DST_BIT | */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, attachment.image, attachment.memory);

	ti->vulkanTexture.deviceMemory = nullptr;
	ti->vulkanTexture.layerCount = 1;
	ti->vulkanTexture.imageFormat = format;
	ti->vulkanTexture.image = attachment.image;
	ti->vulkanTexture.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	ti->vulkanTexture.deviceMemory = attachment.memory;
	ti->vulkanTexture.depth = 1; // TODO check
	//ti->vulkanTexture.imageLayout = 0; // TODO check
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

	if (vkCreateSampler(device, &samplerCI, nullptr, &brdfSampler) != VK_SUCCESS) {
		Error("failed to create brdflut sampler!");
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
	engine->files.readFile("genbrdflut_vert.spv", file_buffer_vert, FileCategory::FX);
	engine->files.readFile("genbrdflut_frag.spv", file_buffer_frag, FileCategory::FX);
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
	vkDestroySampler(device, brdfSampler, nullptr);

	ti->available = true;
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


TextureStore::~TextureStore()
{
	auto& device = engine->global.device;
	for (auto& tex : textures) {
		auto &ti = tex.second;
		Log("Texture found: " << ti.id.c_str() << " " << ti.filename.c_str() << " " << ti.vulkanTexture.deviceMemory << endl);
		if (ti.available) {
			vkDestroyImageView(engine->global.device, tex.second.imageView, nullptr);
			if (ti.isKtxCreated) {
				ktxVulkanTexture_Destruct(&ti.vulkanTexture, engine->global.device, nullptr);
			} else {
				vkDestroyImage(device, ti.vulkanTexture.image, nullptr);
				vkFreeMemory(device, ti.vulkanTexture.deviceMemory, nullptr);
			}
		}
	}
	ktxVulkanDeviceInfo_Destruct(&vdi);
	vkDestroyDescriptorSetLayout(device, layout, nullptr);
	vkDestroyDescriptorPool(device, pool, nullptr);

}

