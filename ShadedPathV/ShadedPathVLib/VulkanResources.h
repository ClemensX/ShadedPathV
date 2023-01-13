#pragma once

// forward declarations
class ShadedPathEngine;

enum class VulkanResourceType {
	MVPBuffer, // base matrices, renewed every frame (will be stored ina desciptor set)
	VertexBufferStatic, // vertex buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
	IndexBufferStatic, // index buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
};

class VulkanResourceElement {
public:
	VulkanResourceType type;
	bool initialized = false;
};



/*
 * handle vulkan shader resources that relate to descriptors
 * specify shader resources and submit them for descriptor and binding set construction
 * 3 things can be bound to a command buffer: vertex buffers, index buffer and descriptor sets.
 * All 3 can be handled through this class. All internmediate structures like DescriptorSetLayout
 * and DescriptorSetLayoutCreateInfo will be created automatically
*/
class VulkanResources
{
public:
	// init texture store
	void init(ShadedPathEngine* engine);

	~VulkanResources();

	// create shader module
	VkShaderModule createShaderModule(std::string filename);

	// Buffers
	void createVertexBufferStatic(std::vector<VulkanResourceElement>& def, size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createIndexBufferStatic(std::vector<VulkanResourceElement>& def, size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

private:
	ShadedPathEngine* engine = nullptr;
};
