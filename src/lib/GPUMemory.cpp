#include "mainheader.h"

using namespace std;

GPUMemory::GPUMemory()
{}

GPUMemory::~GPUMemory()
{
    cleanup();
}

void GPUMemory::init(ShadedPathEngine* engine)
{
    this->engine = engine;
    this->rendering = &engine->globalRendering;
}

void GPUMemory::defineBuffer(const BufferConfiguration& config)
{
    if (buffers.find(config.type) != buffers.end()) {
        Error("GPUMemory::defineBuffer: Buffer type " + getBufferTypeName(config.type) + " already defined");
        return;
    }
    assertProperAlignment(config.elementSize);

    BufferState state;
    state.config = config;

    // Determine if staging buffer is needed based on memory properties
    state.requiresStaging = (config.memoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
        !(config.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // disable staging for None buffer type
    if (config.type == BufferType::None) {
        state.requiresStaging = false;
    }

    buffers[config.type] = state;
    Log("WARNING GPUMemory::defineBuffer: Defined buffer type " + getBufferTypeName(config.type) +
        " with element size " + std::to_string(config.elementSize) +
        ", max element count " + std::to_string(config.maxElementCount) +
        ", usage flags " + std::to_string(config.usage) +
        ", memory properties " + std::to_string(config.memoryProperties) +
        ", requires staging: " + (state.requiresStaging ? "true" : "false") << endl);
}

void GPUMemory::defineBuffer(BufferType type, uint32_t elementSize, uint32_t maxElementCount,
    VkBufferUsageFlags additionalUsageFlags)
{
    BufferConfiguration config;
    config.type = type;
    config.elementSize = elementSize;
    config.maxElementCount = maxElementCount;

    // Default usage flags - always include transfer dst for staging buffer support and shader device address for storage buffers
    config.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | additionalUsageFlags;

    // Default to device-local memory for best performance
    config.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Add shader device address support for storage buffers
    //if (type == StorageBuffer || type == VertexBuffer || type == IndexBuffer) {
    //    config.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    //}

    // Add appropriate buffer type usage flags
    switch (type) {
    case ModelsMoving:
        config.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    case Materials:
    case CollectionIndices:
    case CollectionInfos:
    case MeshInfos:
    case Models:
    case FrameParams:
        config.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;
    case None:
        config.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        config.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    defineBuffer(config);
}

void GPUMemory::allocateBuffers()
{
    for (auto& [type, state] : buffers) {
        createBufferInternal(state);
    }
}

void GPUMemory::flushBuffer(BufferType type)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::flushBuffer: Buffer type " + getBufferTypeName(type) + " not found");
        return;
    }

    if (!state->isDirty) {
        return; // Nothing to flush
    }

    if (state->requiresStaging) {
        // get number of used slots in the buffer
        auto usedElementCount = state->currentElementCount;
        // Copy from staging buffer to device buffer
        VkDeviceSize bufferSize = state->config.elementSize * usedElementCount;

        rendering->copyBuffer(state->stagingBuffer, state->chunk->buffer, bufferSize, state->relDeviceAddress);
        Log("INFO: Flushed buffer " << getBufferTypeName(type) << " from staging to device buffer, size: " << bufferSize << endl);
        // emit warning if flushing the same buffer multiple times:
        if (state->wasAlreadyFlushed) {
            Log("WARNING: Flushed buffer " << getBufferTypeName(type) << " from staging to device buffer multiple times" << endl);
        }
        state->wasAlreadyFlushed = true;
        // emit warning if called during rendering phase
        if (engine->isRenderingPhase()) {
            Log("WARNING: Flushed buffer " << getBufferTypeName(type) << " from staging to device buffer during rendering phase" << endl);
        }
    }
    // For host-visible buffers, data is already in place

    state->isDirty = false;
}

uint64_t GPUMemory::copyToGlobalBuffer(VkDeviceSize bufferSize, const void* src)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // create staging buffer (host-visible)
    rendering->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory, "Staging");

    // copy data to staging buffer
    void* data;
    vkMapMemory(engine->globalRendering.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, src, (size_t)bufferSize);
    vkUnmapMemory(engine->globalRendering.device, stagingBufferMemory);

    // allocate gpu memory
    auto* mem = getCurrentGPUMemoryChunk();
    uint64_t pos = allocate(bufferSize, mem);

    engine->globalRendering.copyBuffer(stagingBuffer, mem->buffer, bufferSize, pos);

    vkDestroyBuffer(engine->globalRendering.device, stagingBuffer, nullptr);
    vkFreeMemory(engine->globalRendering.device, stagingBufferMemory, nullptr);
    return mem->address + pos;
}


void GPUMemory::flushAllBuffers()
{
    for (auto& [type, state] : buffers) {
        if (state.isDirty) {
            flushBuffer(type);
        }
    }
}

VkBuffer GPUMemory::getBuffer(BufferType type) const
{
    const auto* state = getBufferState(type);
    return state ? state->buffer : VK_NULL_HANDLE;
}

uint64_t GPUMemory::getDeviceAddress(BufferType type) const
{
    const auto* state = getBufferState(type);
    uint64_t ret = state && state->chunk ? state->relDeviceAddress + state->chunk->address: 0;
    if (ret == 0) {
        Error("GPUMemory::getDeviceAddress: Buffer type " + getBufferTypeName(type) + " not found or does not have device address");
    }
    return ret;
}

uint32_t GPUMemory::getElementCount(BufferType type) const
{
    const auto* state = getBufferState(type);
    return state ? state->currentElementCount : 0;
}

uint32_t GPUMemory::getMaxElementCount(BufferType type) const
{
    const auto* state = getBufferState(type);
    return state ? state->config.maxElementCount : 0;
}

void GPUMemory::resetElementCount(BufferType type)
{
    auto* state = getBufferState(type);
    if (state) {
        state->currentElementCount = 0;
    }
}

void GPUMemory::cleanup()
{
    if (!rendering) return;

    for (auto& [type, state] : buffers) {
        // Unmap memory if mapped
        if (state.mappedMemory && state.stagingMemory != VK_NULL_HANDLE) {
            vkUnmapMemory(rendering->device, state.stagingMemory);
            state.mappedMemory = nullptr;
        }

        // Destroy staging buffer and memory
        if (state.stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(rendering->device, state.stagingBuffer, nullptr);
            state.stagingBuffer = VK_NULL_HANDLE;
        }
        if (state.stagingMemory != VK_NULL_HANDLE) {
            vkFreeMemory(rendering->device, state.stagingMemory, nullptr);
            state.stagingMemory = VK_NULL_HANDLE;
        }

        // Destroy device buffer and memory
        if (state.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(rendering->device, state.buffer, nullptr);
            state.buffer = VK_NULL_HANDLE;
        }
        if (state.memory != VK_NULL_HANDLE) {
            vkFreeMemory(rendering->device, state.memory, nullptr);
            state.memory = VK_NULL_HANDLE;
        }
    }

    buffers.clear();
    for (auto& chunk : gpuMemoryChunks) {
        if (chunk.buffer != nullptr) {
            vkDestroyBuffer(rendering->device, chunk.buffer, nullptr);
            chunk.buffer = nullptr;
        }
        if (chunk.memory != nullptr) {
            vkFreeMemory(rendering->device, chunk.memory, nullptr);
            chunk.memory = nullptr;
        }
    }
}

// Private helper methods

BufferState* GPUMemory::getBufferState(BufferType type)
{
    auto it = buffers.find(type);
    if (it == buffers.end()) {
        return nullptr;
    }
    return &it->second;
}

const BufferState* GPUMemory::getBufferState(BufferType type) const
{
    auto it = buffers.find(type);
    if (it == buffers.end()) {
        return nullptr;
    }
    return &it->second;
}

void GPUMemory::createBufferInternal(BufferState& state)
{
    VkDeviceSize bufferSize = state.config.elementSize * state.config.maxElementCount;

    if (bufferSize == 0) {
        Error("GPUMemory::createBufferInternal: Buffer size is 0 for type " +
            getBufferTypeName(state.config.type));
        return;
    }

    string debugName = "GPUMemory_" + getBufferTypeName(state.config.type);

    if (state.requiresStaging) {
        //// Create device-local buffer
        //rendering->createBuffer(
        //    bufferSize,
        //    state.config.usage,
        //    state.config.memoryProperties,
        //    state.buffer,
        //    state.memory,
        //    debugName
        //);

        auto* mem = getCurrentGPUMemoryChunk();
        uint64_t pos = allocate(bufferSize, mem);
        state.relDeviceAddress = pos;
        state.chunk = mem;

        // Create staging buffer (host-visible)
        rendering->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            state.stagingBuffer,
            state.stagingMemory,
            debugName + "_Staging"
        );

        // Map staging buffer memory permanently
        if (vkMapMemory(rendering->device, state.stagingMemory, 0, bufferSize, 0, &state.mappedMemory) != VK_SUCCESS) {
            Error("GPUMemory::createBufferInternal: Failed to map staging buffer memory for type " +
                getBufferTypeName(state.config.type));
        }
    } else {
        Error("GPUMemory::createBufferInternal: Direct buffer creation not supported for type " +
            getBufferTypeName(state.config.type) + " because it is not host visible. Consider using staging buffer workflow.");
    }

    // Get device address if requested
    if (state.config.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        //state.deviceAddress = rendering->getBufferDeviceAddress(state.buffer);
        assert(state.chunk != nullptr);
    }
}

void GPUMemory::updateBufferInternal(BufferType type, const void* data, uint32_t elementCount, uint32_t startIndex)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::updateBufferInternal: Buffer type " + getBufferTypeName(type) + " not found");
        return;
    }

    if (!data) {
        Error("GPUMemory::updateBufferInternal: Data pointer is null for type " + getBufferTypeName(type));
        return;
    }

    VkDeviceSize offset = startIndex * state->config.elementSize;
    VkDeviceSize size = elementCount * state->config.elementSize;

    // Copy data to the appropriate buffer
    void* dest = state->mappedMemory;
    if (!dest) {
        Error("GPUMemory::updateBufferInternal: Buffer memory not mapped for type " + getBufferTypeName(type));
        return;
    }

    // Copy to mapped memory (either staging or direct)
    char* destPtr = static_cast<char*>(dest) + offset;
    memcpy(destPtr, data, size);

    // Mark buffer as dirty if using staging
    if (state->requiresStaging) {
        state->isDirty = true;
    }
}

void GPUMemory::checkBounds(BufferType type, uint32_t startIndex, uint32_t count) const
{
    const auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::checkBounds: Buffer type " + getBufferTypeName(type) + " not found");
        return;
    }

    if (startIndex >= state->config.maxElementCount) {
        Error("GPUMemory::checkBounds: Start index " + to_string(startIndex) +
            " exceeds max element count " + to_string(state->config.maxElementCount) +
            " for buffer type " + getBufferTypeName(type));
    }

    if (startIndex + count > state->config.maxElementCount) {
        Error("GPUMemory::checkBounds: Range [" + to_string(startIndex) + ", " +
            to_string(startIndex + count) + ") exceeds max element count " +
            to_string(state->config.maxElementCount) + " for buffer type " + getBufferTypeName(type));
    }
}

string GPUMemory::getBufferTypeName(BufferType type) const
{
    switch (type) {
    case CollectionIndices:    return "CollectionIndices";
    case CollectionInfos:      return "CollectionInfos";
    case MeshInfos:            return "MeshInfos";
    case Models:               return "Models";
    case ModelsMoving:         return "ModelsMoving";
    case Materials:            return "Materials";
    case FrameParams:          return "FrameParams";
    default:                   return "Unknown";
    }
}

void GPUMemory::fillGPUMemoryAddressConstants(GPUMemoryAddressConstants* pushConstants) const
{
    if (!pushConstants) {
        Error("GPUMemory::fillGPUMemoryAddressConstants: pushConstants pointer is null");
        return;
    }

    // Zero out the structure first
    memset(pushConstants, 0, sizeof(GPUMemoryAddressConstants));

    // Fill in device addresses for each buffer type that exists
    //pushConstants->collectionIndicesAddress = getDeviceAddress(CollectionIndices);
    //pushConstants->collectionInfosAddress = getDeviceAddress(CollectionInfos);
    pushConstants->meshInfosAddress = getDeviceAddress(MeshInfos);
    pushConstants->modelsAddress = getDeviceAddress(Models);
    pushConstants->modelsMovingAddress = getDeviceAddress(ModelsMoving);
    pushConstants->materialsAddress = getDeviceAddress(Materials);
    pushConstants->frameParamsBufferAddress = getDeviceAddress(FrameParams);

    //pushConstants->uniformBufferAddress = getDeviceAddress(UniformBuffer);
    //pushConstants->storageBufferAddress = getDeviceAddress(StorageBuffer);
   //pushConstants->vertexBufferAddress = getDeviceAddress(VertexBuffer);
    //pushConstants->indexBufferAddress = getDeviceAddress(IndexBuffer);
    //pushConstants->indirectBufferAddress = getDeviceAddress(IndirectBuffer);
    //pushConstants->textureBufferAddress = getDeviceAddress(TextureBuffer);
    Log("WARNING: GPUMemory::fillGPUMemoryAddressConstants: Filled push constants with buffer addresses: CollectionIndices=" << std::hex << pushConstants->collectionIndicesAddress <<
        ", CollectionInfos=" << pushConstants->collectionInfosAddress <<
        ", MeshInfos=" << pushConstants->meshInfosAddress << std::dec << endl);
}

VkDeviceAddress GPUMemory::getBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfo bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return vkGetBufferDeviceAddress(rendering->device, &bufferDeviceAI);
}

void GPUMemory::createGPUMemoryChunk(VkDeviceSize bufferSize) {
    //VkDeviceSize bufferSize = engine.getMeshStorageSize();
    bufferSize = minAlign(bufferSize, 16);
    GPUMemoryChunk chunk;
    chunk.chunkNumber = (int)gpuMemoryChunks.size();
    std::string dbgName = "GPUMemory global GPU memory chunk " + std::to_string(chunk.chunkNumber);
    rendering->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        chunk.buffer, chunk.memory, dbgName);
    chunk.address = getBufferDeviceAddress(chunk.buffer);
    chunk.nextFreePos = 0;
    chunk.size = bufferSize;
    gpuMemoryChunks.push_back(chunk);
}
