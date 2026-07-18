#pragma once

class ShadedPathEngine;
class GlobalRendering;

// types for named buffers, one buffer per name
enum BufferType {
     // for non-staging buffers
    None = 0,
    // index into CollectionInfos table, size: engine->MaxCollections
    CollectionIndices,
    // collectionInfos table, one entry per main mesh
    CollectionInfos,
    // MeshInfos table, one entry per mesh LOD (10 LODs per main mesh), size: engine->MaxMeshes
    MeshInfos,
    Models,
    ModelsMoving,
    Materials,
    VertexBuffer,
    IndexBuffer,
    IndirectBuffer,
    TextureBuffer
};

// Push constants structure for passing GPU buffer addresses to shaders
// Make sure to match this in shader code (common_cpp_shader.h or similar)
struct GPUMemoryPushConstants {
    uint64_t collectionIndicesAddress;
    uint64_t collectionInfosAddress;
    uint64_t meshInfosAddress;
    uint64_t modelsAddress;
    uint64_t modelsMovingAddress;
    uint64_t materialsAddress;
    uint64_t indexBufferAddress;
    uint64_t indirectBufferAddress;
    uint64_t textureBufferAddress;
};

// Define the push constant range for GPUMemory
const VkPushConstantRange gpuMemoryPushConstantRange = {
    VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
    0,                              // offset
    sizeof(GPUMemoryPushConstants)  // size
};

// for each buffer 'BufferType' we define the structure it uses
struct BufferConfiguration {
    BufferType type;
    uint32_t elementSize;        // size of one element in the buffer, e.g. sizeof(GPUMeshIndex) for MeshIndices
    uint32_t maxElementCount;    // maximum number of elements in the buffer, e.g. max number of meshes for MeshIndices
    VkBufferUsageFlags usage;    // Vulkan buffer usage flags
    VkMemoryPropertyFlags memoryProperties; // memory properties (host visible, device local, etc.)
};

// hold info for GPU memory chunks allocated (only one atm...)
struct GPUMemoryChunk {
    int chunkNumber = -1;
    VkBuffer buffer = nullptr;
    VkDeviceMemory memory = nullptr;
    VkDeviceAddress address = 0;
    uint64_t size = 0;
    uint64_t nextFreePos = 0;
    void reset() { nextFreePos = 0; }
};

// Internal buffer state for each configured buffer type
struct BufferState {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    void* mappedMemory = nullptr;           // for host-visible buffers
    uint64_t relDeviceAddress = 0;          // for shader device address
    uint32_t currentElementCount = 0;       // current number of elements in the buffer
    BufferConfiguration config;
    bool requiresStaging = true;            // true if using device-local memory (default)
    bool isDirty = false;                   // needs flushing to GPU
    bool wasAlreadyFlushed = false;         // has already been flushed to GPU (for warning about multiple flushing)
    GPUMemoryChunk* chunk = nullptr;
};

// maintain GPU memory management
// structured buffers for various data, eventually replacing all special-purpose buffers like UBOs and storage buffers for meshes, materials, etc.
class GPUMemory
{
public:
    GPUMemory();
    ~GPUMemory();

    // Initialize with engine reference
    void init(ShadedPathEngine* engine);

    // ====== INIT PHASE METHODS ======

    // Define a buffer type before rendering
    // Should be called during initialization phase
    // Buffer will be allocated with maxElementCount capacity (not extendable)
    void defineBuffer(const BufferConfiguration& config);

    // Helper method to define buffer with common defaults
    void defineBuffer(BufferType type, uint32_t elementSize, uint32_t maxElementCount,
        VkBufferUsageFlags additionalUsageFlags = 0);

    // Allocate all defined buffers on GPU with their full maxElementCount capacity
    // Must be called after all defineBuffer() calls and before rendering
    void allocateBuffers();

    // in addition to named BufferType buffers, we have simple one-time upload buffers. There is no maintained staging buffer.
    // we use the returned device address in other structures to reference the data in shaders
    uint64_t copyToGlobalBuffer(VkDeviceSize bufferSize, const void* src);

    // Get staging buffer as typed array for C++ side access (read-only)
    // Returns nullptr if buffer doesn't exist, memory is not mapped, or maxIndex exceeds buffer size
    // Note: This returns the staging buffer memory which is host-visible
    // maxIndex: The highest index you plan to access (e.g., if you want to access elements [0..9], pass 9)
    template<typename T>
    const T* getCppBuffer(BufferType type, uint32_t maxIndex) const;

    // Get staging buffer as typed array for C++ side access (mutable)
    // Returns nullptr if buffer doesn't exist, memory is not mapped, or maxIndex exceeds buffer size
    // Note: This returns the staging buffer memory - remember to call flushBuffer() afterwards!
    // maxIndex: The highest index you plan to access (e.g., if you want to access elements [0..9], pass 9)
    template<typename T>
    T* getCppBuffer(BufferType type, uint32_t maxIndex);

    // ====== RENDERING PHASE METHODS ======

    // Fill push constants structure with current buffer addresses
    // Call this once per frame/shader and pass to vkCmdPushConstants
    void fillPushConstants(GPUMemoryPushConstants* pushConstants) const;

    // Copy a single element to the buffer at the specified index
    // Calls Error() if index >= maxElementCount
    // Returns the element index
    template<typename T>
    uint32_t updateElement(BufferType type, const T& element, uint32_t index);

    // Copy multiple elements to the buffer starting at the specified index
    // Calls Error() if startIndex + count > maxElementCount
    // Returns the starting index
    template<typename T>
    uint32_t updateElements(BufferType type, const T* elements, uint32_t count, uint32_t startIndex);

    // Append a single element to the buffer (auto-increments count)
    // Calls Error() if currentElementCount >= maxElementCount
    // Returns the element index
    template<typename T> requires (!std::is_pointer_v<T>)
    uint32_t appendElement(BufferType type, const T& element);

    // Append multiple elements to the buffer (auto-increments count)
    // Calls Error() if currentElementCount + count > maxElementCount
    // Returns the starting index
    template<typename T>
    uint32_t appendElements(BufferType type, const T* elements, uint32_t count);

    // Flush staged changes to GPU (for staging buffer workflow)
    // Call this after updating elements and before rendering with them
    // staging buffer will be maintained, to enable another flush
    void flushBuffer(BufferType type);

    // Flush staged changes to GPU (for staging buffer workflow)
    // Call this after updating elements and before rendering with them
    // staging buffer will be deleted after copying the data
    void flushBufferAndDiscardStaging(BufferType type);

    // Flush all dirty buffers
    void flushAllBuffers();

    // ====== QUERY METHODS ======

    // Get the Vulkan buffer handle
    VkBuffer getBuffer(BufferType type) const;

    // Get the buffer device address (for shader access)
    uint64_t getDeviceAddress(BufferType type) const;

    // Get current element count
    uint32_t getElementCount(BufferType type) const;

    // Get maximum element count
    uint32_t getMaxElementCount(BufferType type) const;

    // Reset element count to 0 (doesn't clear memory, just resets the counter)
    void resetElementCount(BufferType type);

    // Get CPU-accessible address of an element in the staging buffer
    // Returns nullptr if:
    // - Buffer doesn't exist
    // - No staging buffer available (not using device-local memory)
    // - Memory is not mapped
    // - Index is out of bounds
    // Use this for direct memory writes, but remember to call flushBuffer() afterwards
    template<typename T>
    T* getElementAddress(BufferType type, uint32_t index);

    // Get CPU-accessible address of a range of elements
    // Same restrictions as getElementAddress()
    template<typename T>
    T* getElementsAddress(BufferType type, uint32_t startIndex, uint32_t count);
    
    // ====== CLEANUP ======

    // Destroy all buffers and free memory
    void cleanup();

    // ====== UTILITY ======

    // Calculate aligned size for arrays based on element size
    static uint32_t calculateArrayStride(uint32_t elementSize) {
        // std430 rules:
        // - Scalars/vectors: natural alignment (4, 8, 16)
        // - Structures: align to largest member, round up to 16 if > 16
        // - Arrays: stride = aligned element size

        if (elementSize <= 4) return 4;
        if (elementSize <= 8) return 8;
        if (elementSize <= 16) return 16;

        // For larger structures, round up to multiple of 16
        return (elementSize + 15) & ~15; // Round up to 16-byte boundary
    }

    // we need properly aligned buffers for arrays of structures, so make sure array elements are already properly aligned on C++ side
    static void assertProperAlignment(uint32_t elementSize) {
        return;
        if (calculateArrayStride) {
            Error("GPUMemory: Element size " + std::to_string(elementSize) + " is not properly aligned for array storage. Must be 4 or 8 or a multiple of 16.");
        }
    }

    // ====== GPU memory chunks ======

    VkDeviceSize minAlign(VkDeviceSize size, VkDeviceSize alignment)
    {
        if (alignment == 0) {
            return size; // no alignment needed
        }
        if (size % alignment == 0) {
            return size; // already aligned
        }
        return ((size + alignment - 1) / alignment) * alignment; // round up to next multiple of alignment
    }

    // get current GPU memory chunk, currently we do not allocate another one if the first is full...
    GPUMemoryChunk* getCurrentGPUMemoryChunk() {
        if (gpuMemoryChunks.size() == 0) {
            Error("No GPU memory chunk allocated");
            return nullptr;
        }
        return &gpuMemoryChunks[0];
    }
    uint64_t allocate(uint64_t size, GPUMemoryChunk* chunk)
    {
        size = minAlign(size, 16);
        if (chunk->nextFreePos + size > chunk->size) {
            Error("Global Rendering: out of global mesh storage memory. Increase in engine settings or allocate new chunk.");
        }
        uint64_t ret = chunk->nextFreePos;
        chunk->nextFreePos += size;
        return ret;
    }

    VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer);
    void createGPUMemoryChunk(VkDeviceSize bufferSize);

    std::vector<GPUMemoryChunk> gpuMemoryChunks;



private:
    ShadedPathEngine* engine = nullptr;
    GlobalRendering* rendering = nullptr;

    std::unordered_map<BufferType, BufferState> buffers;

    // Helper methods
    BufferState* getBufferState(BufferType type);
    const BufferState* getBufferState(BufferType type) const;
    void createBufferInternal(BufferState& state);
    void updateBufferInternal(BufferType type, const void* data, uint32_t elementCount, uint32_t startIndex);
    void checkBounds(BufferType type, uint32_t startIndex, uint32_t count) const;

    std::string getBufferTypeName(BufferType type) const;
};

// Template implementations
template<typename T>
inline uint32_t GPUMemory::updateElement(BufferType type, const T& element, uint32_t index)
{
    checkBounds(type, index, 1);
    updateBufferInternal(type, &element, 1, index);
    return index;
}

template<typename T>
inline uint32_t GPUMemory::updateElements(BufferType type, const T* elements, uint32_t count, uint32_t startIndex)
{
    checkBounds(type, startIndex, count);
    updateBufferInternal(type, elements, count, startIndex);
    return startIndex;
}

template<typename T> requires (!std::is_pointer_v<T>)
inline uint32_t GPUMemory::appendElement(BufferType type, const T& element)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::appendElement: Buffer type " + getBufferTypeName(type) + " not found");
        return 0;
    }

    checkBounds(type, state->currentElementCount, 1);

    uint32_t index = state->currentElementCount;
    updateBufferInternal(type, &element, 1, index);
    state->currentElementCount++;
    return index;
}

template<typename T>
inline uint32_t GPUMemory::appendElements(BufferType type, const T* elements, uint32_t count)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::appendElements: Buffer type " + getBufferTypeName(type) + " not found");
        return 0;
    }
    
    checkBounds(type, state->currentElementCount, count);
    
    uint32_t startIndex = state->currentElementCount;
    updateBufferInternal(type, elements, count, startIndex);
    state->currentElementCount += count;
    return startIndex;
}

template<typename T>
inline T* GPUMemory::getElementAddress(BufferType type, uint32_t index)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::getElementAddress: Buffer type " + getBufferTypeName(type) + " not found");
        return nullptr;
    }
    
    // Check if staging buffer is available
    if (!state->requiresStaging && !(state->config.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        Error("GPUMemory::getElementAddress: Buffer type " + getBufferTypeName(type) + 
              " has no CPU-accessible memory (not host-visible and no staging buffer)");
        return nullptr;
    }
    
    // Check if memory is mapped
    if (!state->mappedMemory) {
        Error("GPUMemory::getElementAddress: Buffer type " + getBufferTypeName(type) + " memory is not mapped");
        return nullptr;
    }
    
    // Check bounds
    if (index >= state->config.maxElementCount) {
        Error("GPUMemory::getElementAddress: Index " + std::to_string(index) + 
              " out of bounds for buffer type " + getBufferTypeName(type) + 
              " (max: " + std::to_string(state->config.maxElementCount) + ")");
        return nullptr;
    }
    
    // Verify element size matches
    if (sizeof(T) != state->config.elementSize) {
        Error("GPUMemory::getElementAddress: Template type size " + std::to_string(sizeof(T)) + 
              " does not match configured element size " + std::to_string(state->config.elementSize) + 
              " for buffer type " + getBufferTypeName(type));
        return nullptr;
    }
    
    // Calculate offset and return pointer
    VkDeviceSize offset = index * state->config.elementSize;
    char* basePtr = static_cast<char*>(state->mappedMemory);
    return reinterpret_cast<T*>(basePtr + offset);
}

template<typename T>
inline T* GPUMemory::getElementsAddress(BufferType type, uint32_t startIndex, uint32_t count)
{
    auto* state = getBufferState(type);
    if (!state) {
        Error("GPUMemory::getElementsAddress: Buffer type " + getBufferTypeName(type) + " not found");
        return nullptr;
    }
    
    // Check if staging buffer is available
    if (!state->requiresStaging && !(state->config.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        Error("GPUMemory::getElementsAddress: Buffer type " + getBufferTypeName(type) + 
              " has no CPU-accessible memory (not host-visible and no staging buffer)");
        return nullptr;
    }
    
    // Check if memory is mapped
    if (!state->mappedMemory) {
        Error("GPUMemory::getElementsAddress: Buffer type " + getBufferTypeName(type) + " memory is not mapped");
        return nullptr;
    }
    
    // Check bounds
    if (startIndex + count > state->config.maxElementCount) {
        Error("GPUMemory::getElementsAddress: Range [" + std::to_string(startIndex) + ", " + 
              std::to_string(startIndex + count) + ") exceeds max element count " + 
              std::to_string(state->config.maxElementCount) + " for buffer type " + getBufferTypeName(type));
        return nullptr;
    }
    
    // Verify element size matches
    if (sizeof(T) != state->config.elementSize) {
        Error("GPUMemory::getElementsAddress: Template type size " + std::to_string(sizeof(T)) + 
              " does not match configured element size " + std::to_string(state->config.elementSize) + 
              " for buffer type " + getBufferTypeName(type));
        return nullptr;
    }
    
    // Calculate offset and return pointer
    VkDeviceSize offset = startIndex * state->config.elementSize;
    char* basePtr = static_cast<char*>(state->mappedMemory);
    return reinterpret_cast<T*>(basePtr + offset);
}

template<typename T>
inline const T* GPUMemory::getCppBuffer(BufferType type, uint32_t maxIndex) const
{
    auto* state = getBufferState(type);
    if (!state || !state->mappedMemory) {
        return nullptr;
    }

    // Verify element size matches
    if (sizeof(T) != state->config.elementSize) {
        Error("GPUMemory::getCppBuffer: Template type size " + std::to_string(sizeof(T)) +
            " does not match configured element size " + std::to_string(state->config.elementSize) +
            " for buffer type " + getBufferTypeName(type));
        return nullptr;
    }

    // Check that maxIndex is within bounds
    if (maxIndex >= state->config.maxElementCount) {
        Error("GPUMemory::getCppBuffer: maxIndex " + std::to_string(maxIndex) +
            " exceeds max element count " + std::to_string(state->config.maxElementCount) +
            " for buffer type " + getBufferTypeName(type));
        return nullptr;
    }

    return reinterpret_cast<const T*>(state->mappedMemory);
}

template<typename T>
inline T* GPUMemory::getCppBuffer(BufferType type, uint32_t maxIndex)
{
    return const_cast<T*>(static_cast<const GPUMemory*>(this)->getCppBuffer<T>(type, maxIndex));
}