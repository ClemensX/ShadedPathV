#include "mainheader.h"

using namespace std;

void VulkanResources::init(ShadedPathEngine* engine) {
	Log("VulkanResources c'tor\n");
	this->engine = engine;
    bufferInfos.reserve(10); // avoid resize as we store pointers to elements of this vector
}

VkShaderModule VulkanResources::createShaderModule(string filename)
{
    vector<byte> file_buffer;
    engine->files.readFile(filename, file_buffer, FileCategory::FX);
    //Log("read shader: " << file_buffer.size() << endl);
    // create shader modules
    VkShaderModule shaderModule = engine->shaders.createShaderModule(file_buffer);
    return shaderModule;
}

size_t VulkanResources::getResourceDefIndex(VulkanResourceType t)
{
    for (size_t i = 0; i < resourceDefinition->size(); i++) {
        auto& rd = resourceDefinition ->at(i);
        if (rd.type == t) {
            return i;
        }
    }
    Error("Expected resource type not found in resourceDefinition vector");
    return -1; // keep compiler happy
}

void VulkanResources::createVertexBufferStatic(VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    vector<VulkanResourceElement> &def = *resourceDefinition;
    size_t bufferId = getResourceDefIndex(VulkanResourceType::VertexBufferStatic);
    this->engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory, "VertexBufferStatic");
}

void VulkanResources::createIndexBufferStatic(VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;
    size_t bufferId = getResourceDefIndex(VulkanResourceType::IndexBufferStatic);
    this->engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bufferSize, src, buffer, bufferMemory, "IndexBufferStatic");
}

void VulkanResources::createDescriptorSetResources(VkDescriptorSetLayout& layout, VkDescriptorPool& pool, ShaderBase* shaderBase, int poolMaxSetsFactor)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;

    for (auto& d : def) {
        addResourcesForElement(d);
    }

    //VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    //VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info{};
    //flag_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    //flag_info.bindingCount = static_cast<uint32_t>(bindings.size());
    //flag_info.pBindingFlags = &flag;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //layoutInfo.pNext = &flag_info;
    layoutInfo.pNext = nullptr;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(engine->globalRendering.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout!");
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = engine->getFramesInFlight() * poolMaxSetsFactor; // max calls to vkCreateDescriptorPool
    if (engine->isStereo()) {
        poolInfo.maxSets *= 2;
    }
    // TODO hack until we have switched to global texture array
    for (auto& rd : *resourceDefinition) {
        if (rd.type == VulkanResourceType::UniformBufferDynamic) {
            poolInfo.maxSets *= 2;
        }
    }

    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    shaderBase->poolAllocationMaxSet = poolInfo.maxSets;
    // multiply each descriptor count by number of sets (strangely, Vulkan uses the single descriptor count for all sets)
    for (auto& p : poolSizes) {
        Log("pool add single descriptors " << shaderBase->getName() << ": " << p.descriptorCount << endl);
        p.descriptorCount *= poolInfo.maxSets;
        shaderBase->poolAllocationMaxSingleDescriptors += p.descriptorCount;
    }
    Log("pool descriptor sets max count " << shaderBase->getName() << ": " << shaderBase->poolAllocationMaxSet << endl);
    Log("pool total single descriptors " << shaderBase->getName() << ": " << shaderBase->poolAllocationMaxSingleDescriptors << endl);
    shaderBase->poolAllocationMaxSet = poolInfo.maxSets;
    if (vkCreateDescriptorPool(engine->globalRendering.device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        Error("failed to create descriptor pool! Look logs for descriptor couns. Former problem #define OVERRIDE_UNIFORM_BUFFER_DESCRIPTOR_COUNT");
    }
    string name = shaderBase->getName() + " Descriptor Pool";
    engine->util.debugNameObjectDescriptorPool(pool, name.c_str());
    // each shader has its own descriptor set layout and pool, no need to assign here
    //this->layout = layout;
    //this->pool = pool;
}

void VulkanResources::addResourcesForElement(VulkanResourceElement el)
{
    uint32_t bindingCount = static_cast<uint32_t>(bindings.size());
    VkDescriptorSetLayoutBinding layoutBinding{};
    VkDescriptorPoolSize poolSize{};
    if (el.type == VulkanResourceType::MVPBuffer) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT| VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
        if (addGeomShaderStageToMVP) {
            layoutBinding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        // https://community.khronos.org/t/vk-error-out-of-pool-memory-when-allocating-second-descriptor-sets/104304/3
        // https://www.reddit.com/r/vulkan/comments/12nwl3i/out_of_pool_memory_when_allocating_multiple/
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);
        if (engine->isStereo()) {
            poolSizes.push_back(poolSize);
        }
    } else if (el.type == VulkanResourceType::UniformBufferDynamic) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT |VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);

    } else if (el.type == VulkanResourceType::AdditionalUniformBuffer) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT |VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);

    }
    else if (el.type == VulkanResourceType::MeshShader) {
        for (int i = 0; i < 4; i++) {
            layoutBinding.binding = bindingCount + i;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBinding.pImmutableSamplers = nullptr;
            bindings.push_back(layoutBinding);
            poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSize.descriptorCount = 1;
            poolSizes.push_back(poolSize);
        }
        layoutBinding.binding = bindingCount;

    }
    else if (el.type == VulkanResourceType::SingleTexture) {
        layoutBinding.binding = bindingCount;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);
    } else if (el.type == VulkanResourceType::GlobalTextureSet) {
        createDescriptorSetResourcesForTextures();
    }
}

void VulkanResources::createThreadResources(VulkanHandoverResources& hdv)
{
    vector<VulkanResourceElement>& def = *resourceDefinition;
    if (hdv.descriptorSet == nullptr) Error("Shader did not initialize needed Thread Resources: descriptorSet is missing");
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = hdv.shader->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &hdv.shader->descriptorSetLayout;
    hdv.shader->poolAllocationSetCount += allocInfo.descriptorSetCount;
    Log("alloc descriptor set total " << hdv.shader->poolAllocationSetCount << endl);
    VkResult res = vkAllocateDescriptorSets(engine->globalRendering.device, &allocInfo, hdv.descriptorSet);
    if ( res != VK_SUCCESS) {
        if (res == VK_ERROR_OUT_OF_POOL_MEMORY) {
            stringstream s;
            s << "VK_ERROR_OUT_OF_POOL_MEMORY at descriptor allocation. Check your shader implemenation " << hdv.shader->getName();
            Error(s.str().c_str());
        }
        Error("failed to allocate descriptor sets!");
    }
    string dname = hdv.debugBaseName + " Descriptor Set 1";
    engine->util.debugNameObjectDescriptorSet(*hdv.descriptorSet, dname.c_str());
    if (engine->isStereo()) {
        if (hdv.descriptorSet2 == nullptr) Error("Shader did not initialize needed Thread Resources: descriptorSet 2 is missing");
        if (vkAllocateDescriptorSets(engine->globalRendering.device, &allocInfo, hdv.descriptorSet2) != VK_SUCCESS) {
            Error("failed to allocate descriptor sets!");
        }
        string dname = hdv.debugBaseName + " Descriptor Set 2";
        engine->util.debugNameObjectDescriptorSet(*hdv.descriptorSet2, dname.c_str());
    }

    for (auto& d : def) {
        addThreadResourcesForElement(d, hdv);
    }
    vkUpdateDescriptorSets(engine->globalRendering.device, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
    //if (engine->isStereo()) {
    //    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    //}
    descriptorSets.clear();
    bufferInfos.clear();
    imageInfos.clear();
}

void VulkanResources::addThreadResourcesForElement(VulkanResourceElement el, VulkanHandoverResources& hdv)
{
    VkWriteDescriptorSet descSet{};
    if (hdv.descriptorSet == nullptr) Error("Shader did not initialize needed Thread Resources: descriptorSet is missing");

    if (el.type == VulkanResourceType::MVPBuffer) {
        assert(bufferInfos.capacity() >= bufferInfos.size() + 2); // make sure we have enough room
        if (hdv.mvpBuffer == nullptr) Error("Shader did not initialize needed Thread Resources: mvpBuffer is missing");
        if (hdv.mvpSize == 0L) Error("Shader did not initialize needed Thread Resources: mvpSize is missing");
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = hdv.mvpBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = hdv.mvpSize;
        bufferInfos.push_back(bufferInfo);
        if (engine->isStereo()) {
            bufferInfo.buffer = hdv.mvpBuffer2;
            bufferInfos.push_back(bufferInfo);
        }

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 0;
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descSet.descriptorCount = 1;
        descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
        if (engine->isStereo()) {
            descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 2];
        }
        descriptorSets.push_back(descSet);
        if (engine->isStereo()) {
            descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
            descSet.dstSet = *hdv.descriptorSet2;
            descriptorSets.push_back(descSet);
        }
    }
    else if (el.type == VulkanResourceType::UniformBufferDynamic) {
        assert(bufferInfos.capacity() >= bufferInfos.size() + 1); // make sure we have enough room
        if (hdv.dynBuffer == nullptr) Error("Shader did not initialize needed Thread Resources: dynBuffer is missing");
        if (hdv.dynBufferSize == 0L) Error("Shader did not initialize needed Thread Resources: dynBufferSize is missing");
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = hdv.dynBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = hdv.dynBufferSize;
        bufferInfos.push_back(bufferInfo);

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 1;
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descSet.descriptorCount = 1;
        descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
        descriptorSets.push_back(descSet);
        if (engine->isStereo()) {
            descSet.dstSet = *hdv.descriptorSet2;
            descriptorSets.push_back(descSet);
        }
    } else if (el.type == VulkanResourceType::SingleTexture) {
        if (hdv.imageView == nullptr) return;
        if (hdv.imageView == nullptr) Error("Shader did not initialize needed Thread Resources: imageView is missing");
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = hdv.imageView;
        //imageInfo.sampler = engine->globalRendering.textureSampler[(int)TextureType::TEXTURE_TYPE_MIPMAP_IMAGE];
        imageInfo.sampler = engine->globalRendering.textureSampler_TEXTURE_TYPE_MIPMAP_IMAGE;
        imageInfos.push_back(imageInfo);

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 1;
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descSet.descriptorCount = 1;
        descSet.pImageInfo = &imageInfos[imageInfos.size() - 1];
        descriptorSets.push_back(descSet);
        if (engine->isStereo()) {
            descSet.dstSet = *hdv.descriptorSet2;
            descriptorSets.push_back(descSet);
        }
    } else if (el.type == VulkanResourceType::AdditionalUniformBuffer) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = hdv.addBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = hdv.addBufferSize;
        bufferInfos.push_back(bufferInfo);

        descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descSet.dstSet = *hdv.descriptorSet;
        descSet.dstBinding = 2; // after UniformBufferDynamic
        descSet.dstArrayElement = 0;
        descSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descSet.descriptorCount = 1;
        descSet.pBufferInfo = &bufferInfos[bufferInfos.size() - 1];
        descriptorSets.push_back(descSet);
    }
    else if (el.type == VulkanResourceType::MeshShader) {
        // storage buffer for mesh shader descriptors is a per object resource, so we cannot create a global mesh shader buffer
        // instead, we create a storage buffer for each object that uses the mesh shader.
        // then we update the descriptor set with the buffer info.
        //assert(hdv.meshDescBuffer != nullptr);
        //assert(hdv.meshDescBufferSize > 0L);
        //VkDescriptorBufferInfo bufferInfo{};
        //bufferInfo.buffer = hdv.meshDescBuffer;
        //bufferInfo.offset = 0;
        //bufferInfo.range = hdv.meshDescBufferSize;
        //bufferInfos.push_back(bufferInfo);

        //descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //descSet.dstSet = *hdv.descriptorSet;
        //descSet.dstBinding = 2; // after AdditionalUniformBuffer
        //descSet.dstArrayElement = 0;
        //descSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        //descSet.descriptorCount = 1;
        //descSet.pBufferInfo = nullptr;
        //descriptorSets.push_back(descSet);
    }
}

void VulkanResources::createDescriptorSetResourcesForTextures()
{
    // check if already initialized
    if (engine->textureStore.layout != nullptr) return;

    VkDescriptorSetLayoutBinding layoutBinding{};
    VkDescriptorPoolSize poolSize{};

    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = static_cast<uint32_t>(engine->textureStore.getMaxSize());
    layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    layoutBinding.pImmutableSamplers = nullptr;
    textureBindings.push_back(layoutBinding);
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = layoutBinding.descriptorCount;
    texturePoolSizes.push_back(poolSize);

    VkDescriptorBindingFlags flag =
        //VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info{};
    flag_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flag_info.bindingCount = 1;
    flag_info.pBindingFlags = &flag;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &flag_info;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
    layoutInfo.pBindings = textureBindings.data();

    if (vkCreateDescriptorSetLayout(engine->globalRendering.device, &layoutInfo, nullptr, &engine->textureStore.layout) != VK_SUCCESS) {
        Error("failed to create descriptor set layout for textures!");
    }
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(texturePoolSizes.size());
    poolInfo.pPoolSizes = texturePoolSizes.data();
    poolInfo.maxSets = 1; // only one global list for all shaders
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    if (vkCreateDescriptorPool(engine->globalRendering.device, &poolInfo, nullptr, &engine->textureStore.pool) != VK_SUCCESS) {
        Error("failed to create descriptor pool for textures!");
    }
}

void VulkanResources::updateDescriptorSetForTextures(ShadedPathEngine* engine) {
    if (engine->textureStore.pool == nullptr) Error("Adding textures requires initialized engine and shaders! Move to app.init()");
    if (engine->textureStore.descriptorSet == nullptr) {
        // create DescriptorSet
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = engine->textureStore.pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &engine->textureStore.layout;
        VkResult res = vkAllocateDescriptorSets(engine->globalRendering.device, &allocInfo, &engine->textureStore.descriptorSet);
        if (res != VK_SUCCESS) {
            Error("failed to allocate descriptor sets!");
        }
    }

    // iterate over all textures and write descriptor sets
    // use empty local vectors
    size_t numTextures = engine->textureStore.getTexturesMap().size();
    vector<VkDescriptorImageInfo> imageInfos(numTextures);
    vector<VkWriteDescriptorSet> descriptorSets;

    TextureInfo* lastTexture = nullptr;
    for (auto& texMapEntry : engine->textureStore.getTexturesMap()) {
        auto& tex = texMapEntry.second;
        //Log("tex: " << tex.id.c_str() << " index: " << tex.index << " ");
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = tex.imageView;
        //imageInfo.sampler = engine->globalRendering.textureSampler[(int)tex.type];
        if (tex.type == TextureType::TEXTURE_TYPE_GLTF) {
            imageInfo.sampler = tex.sampler;
            //Log("using glTF sampler\n");
        } else {
            imageInfo.sampler = engine->globalRendering.textureSampler_TEXTURE_TYPE_MIPMAP_IMAGE;
            //Log("using default sampler\n");
        }
        imageInfos[tex.index] = imageInfo;
        lastTexture = (TextureInfo*) &tex;
    }
    VkWriteDescriptorSet descSet{};
    descSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descSet.dstSet = engine->textureStore.descriptorSet;
    descSet.dstBinding = 0;
    descSet.dstArrayElement = 0;
    descSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descSet.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    descSet.pImageInfo = &imageInfos[0];

    descriptorSets.push_back(descSet);
    vkUpdateDescriptorSets(engine->globalRendering.device, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
}

void VulkanResources::updateDescriptorSets(FrameResources& tr)
{
    //vector<VulkanResourceElement>& def = *resourceDefinition;
    //for (auto& d : def) {
    //    if (d.type == VulkanResourceType::GlobalTextureSet) {
    //        updateDescriptorSetForTextures(engine);
    //    }

    //}
}

// create pipeline layout and store in parameter.
// auto add set layout for global texture array if needed
void VulkanResources::createPipelineLayout(VkPipelineLayout* pipelineLayout, ShaderBase* shaderBase, VkDescriptorSetLayout additionalLayout, size_t additionalLayoutPos) {
    vector<VkDescriptorSetLayout> sets;
    assert(shaderBase->descriptorSetLayout != nullptr);
    sets.push_back(shaderBase->descriptorSetLayout);
    if (additionalLayout != nullptr) {
        assert(additionalLayoutPos = 1);
        sets.push_back(additionalLayout);
    } 
    if (engine->textureStore.layout) {
        sets.push_back(engine->textureStore.layout);
    }
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(sets.size());
    pipelineLayoutInfo.pSetLayouts = &sets[0];
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    if (shaderBase->pushConstantRanges.size() > 0) {
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(shaderBase->pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = shaderBase->pushConstantRanges.data();
    }

    if (vkCreatePipelineLayout(engine->globalRendering.device, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS) {
        Error("failed to create pipeline layout!");
    }
}


VulkanResources::~VulkanResources()
{
	Log("VulkanResources destructor\n");
}