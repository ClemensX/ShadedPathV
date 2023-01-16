#pragma once

// forward declarations
class ShadedPathEngine;

enum class VulkanResourceType {
	MVPBuffer, // base matrices, renewed every frame (will be stored ina desciptor set)
	SingleTexture, // single read-only texture (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER), stored in descriptor set
	VertexBufferStatic, // vertex buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
	IndexBufferStatic, // index buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
};

class VulkanResourceElement {
public:
	VulkanResourceType type;
	bool initialized = false;
};

/*
 * Example definition: 
 * start with elements of the descriptor set and but vertex and index buffer last.
 * order is important as this will directly correspond with shader module bindings
 * 
	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::SingleTexture },
		{ VulkanResourceType::VertexBufferStatic },
		{ VulkanResourceType::IndexBufferStatic }
	};


*/


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

	// specify resources
	void setResourceDefinition(std::vector<VulkanResourceElement>* def) {
		resourceDefinition = def;
	}

	// create shader module
	VkShaderModule createShaderModule(std::string filename);

	// Buffers
	void createVertexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createIndexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void createDescriptorSetResources(VkDescriptorSetLayout& layout, VkDescriptorPool& pool);
	void addSetLayoutBinding(VulkanResourceElement el);
private:
	ShadedPathEngine* engine = nullptr;
	std::vector<VulkanResourceElement>* resourceDefinition = nullptr;
	std::vector <VkDescriptorSetLayoutBinding> bindings;

};
