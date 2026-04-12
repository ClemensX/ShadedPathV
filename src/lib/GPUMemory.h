#pragma once

class ShadedPathEngine;
class GlobalRendering;

enum BufferType {
    // for each mesh, we have an array of 10 here 
    MeshIndices,
    MeshInfos,
    UniformBuffer,
    StorageBuffer,
    VertexBuffer,
    IndexBuffer,
    IndirectBuffer,
    TextureBuffer
};

// Push constants structure for passing GPU buffer addresses to shaders
// Make sure to match this in shader code (common_cpp_shader.h or similar)
struct GPUMemoryPushConstants {
    uint64_t meshIndicesAddress;
    uint64_t meshInfosAddress;
    uint64_t uniformBufferAddress;
    uint64_t storageBufferAddress;
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    uint64_t indirectBufferAddress;
    uint64_t textureBufferAddress;
};

// Define the push constant range for GPUMemory
const VkPushConstantRange gpuMemoryPushConstantRange = {
    VK_SHADER_STAGE_ALL_GRAPHICS,  // Available to all shader stages
    0,                              // offset
    sizeof(GPUMemoryPushConstants)  // size
};

// for each buffer type we define the structure it uses
struct BufferConfiguration {
    BufferType type;
    uint32_t elementSize;        // size of one element in the buffer, e.g. sizeof(GPUMeshIndex) for MeshIndices
    uint32_t maxElementCount;    // maximum number of elements in the buffer, e.g. max number of meshes for MeshIndices
    VkBufferUsageFlags usage;    // Vulkan buffer usage flags
    VkMemoryPropertyFlags memoryProperties; // memory properties (host visible, device local, etc.)
};

// Internal buffer state for each configured buffer type
struct BufferState {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    void* mappedMemory = nullptr;           // for host-visible buffers
    uint64_t deviceAddress = 0;             // for shader device address
    uint32_t currentElementCount = 0;       // current number of elements in the buffer
    BufferConfiguration config;
    bool requiresStaging = true;            // true if using device-local memory (default)
    bool isDirty = false;                   // needs flushing to GPU
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
    template<typename T>
    uint32_t appendElement(BufferType type, const T& element);

    // Append multiple elements to the buffer (auto-increments count)
    // Calls Error() if currentElementCount + count > maxElementCount
    // Returns the starting index
    template<typename T>
    uint32_t appendElements(BufferType type, const T* elements, uint32_t count);

    // Flush staged changes to GPU (for staging buffer workflow)
    // Call this after updating elements and before rendering with them
    void flushBuffer(BufferType type);

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

template<typename T>
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
