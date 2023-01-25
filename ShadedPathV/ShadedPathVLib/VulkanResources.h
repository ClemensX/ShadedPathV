#pragma once

// forward declarations
class ShadedPathEngine;

enum class VulkanResourceType {
	MVPBuffer, // base matrices, renewed every frame (will be stored ina desciptor set)
	SingleTexture, // single read-only texture (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER), stored in descriptor set
	GlobalTextureSet, // all textures of TextureStore in an indexed, unbound descriptor
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

// thread resources for vkUpdateDescriptorSets
// contains resources for all parts defined in vulkanResourceDefinition
struct VulkanHandoverResources {
	VkDescriptorSet *descriptorSet = nullptr; // PTR!
	VkBuffer mvpBuffer = nullptr;
	VkDeviceSize mvpSize = 0L;
	VkImageView imageView = nullptr;
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

	// specify resources
	void setResourceDefinition(std::vector<VulkanResourceElement>* def) {
		resourceDefinition = def;
	}

	// create shader module
	VkShaderModule createShaderModule(std::string filename);

	// Buffers
	void createVertexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createIndexBufferStatic(size_t bufferId, VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	// create DescriptorSetLayout and DescriptorPool with enough size for all threads
	void createDescriptorSetResources(VkDescriptorSetLayout& layout, VkDescriptorPool& pool);
	// populate descriptor set, all necessary resources have to be set in handover struct
	void createThreadResources(VulkanHandoverResources& res);
private:
	ShadedPathEngine* engine = nullptr;
	std::vector<VulkanResourceElement>* resourceDefinition = nullptr;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorPoolSize> poolSizes;
	std::vector<VkWriteDescriptorSet> descriptorSets;

	void addResourcesForElement(VulkanResourceElement el);
	void addThreadResourcesForElement(VulkanResourceElement d, VulkanHandoverResources& res);
	VkDescriptorSetLayout layout = nullptr;
	VkDescriptorPool pool = nullptr;
	// resources for temporary store info objects between the various create... calls:
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
};