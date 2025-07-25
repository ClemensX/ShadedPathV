#pragma once

// forward declarations
class ShadedPathEngine;


enum class VulkanResourceType {
	// first VkDescriptorSetLayout:
	MVPBuffer, // base matrices, renewed every frame (will be stored in desciptor set)
	UniformBufferDynamic, // dynamic buffer that can be indexed in shader code or during cmd binding
	SingleTexture, // single read-only texture (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER), stored in descriptor set
	AdditionalUniformBuffer, // another UBO, independent form MVP, stored in descriptor set
	MeshShader, // meshlet descriptor storage buffer, ...
	// vertex and index buffers
	VertexBufferStatic, // vertex buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
	IndexBufferStatic, // index buffer, once uplodaded during init phase, only read during frame rendering (will be bound to command buffer)
	// second VkDescriptorSetLayout
	GlobalTextureSet, // all textures of TextureStore in an indexed, unbound descriptor in its own DescriptorSetLayout
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
	VkDescriptorSet* descriptorSet = nullptr; // PTR!
	VkDescriptorSet* descriptorSet2 = nullptr; // PTR! for stereo
	VkBuffer mvpBuffer = nullptr;
	VkBuffer mvpBuffer2 = nullptr;  // for stereo
	VkDeviceSize mvpSize = 0L;
	VkImageView imageView = nullptr;
	VkBuffer dynBuffer = nullptr;
	VkDeviceSize dynBufferSize = 0L;
	ShaderBase* shader = nullptr;
	// additional buffer:
	VkBuffer addBuffer = nullptr;
	VkDeviceSize addBufferSize = 0L;
    // mesh shader: (needs to be per object, we cannot globally create a mesh shader buffer)
	//VkBuffer meshDescBuffer = nullptr;
	//VkDeviceSize meshDescBufferSize = 0L;
	// debug
	std::string debugBaseName = "no_name_given";
    uint32_t debugDescriptorCount = 0;
};


/*
 * handle vulkan shader resources that relate to descriptors
 * specify shader resources and submit them for descriptor and binding set construction
 * 3 things can be bound to a command buffer: vertex buffers, index buffer and descriptor sets.
 * All 3 can be handled through this class. All intermediate structures like DescriptorSetLayout
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
	void createVertexBufferStatic(VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createIndexBufferStatic(VkDeviceSize bufferSize, const void* src, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	// create DescriptorSetLayout and DescriptorPool with enough size for all threads
	// poolMaxSetsFactor is usually the number of sub shaders per render thread
	void createDescriptorSetResources(VkDescriptorSetLayout& layout, VkDescriptorPool& pool, ShaderBase* shaderBase, int poolMaxSetsFactor = 1);
	// populate descriptor set, all necessary resources have to be set in handover struct.
	// Called one-time only during init phase or after init phase in createCommandBuffer().
	// vkAllocateDescriptorSets and create the VkWriteDescriptorSets
	void createThreadResources(VulkanHandoverResources& res);
	// set after init and possibly before each frame
	void updateDescriptorSets(FrameResources& tr);
	// create pipeline layout and store in parameter.
	void createPipelineLayout(VkPipelineLayout* pipelineLayout, ShaderBase* shaderBase, VkDescriptorSetLayout additionalLayout = nullptr, size_t additionalLayoutPos = 0);

	// signal use of geom shader for MVP buffer
	void addGeometryShaderStageToMVPBuffer() {
		this->addGeomShaderStageToMVP = true;
	}
    // update texture descriptors, after a texture has been added or removed
	static void updateDescriptorSetForTextures(ShadedPathEngine* engine);

private:
	ShadedPathEngine* engine = nullptr;
	std::vector<VulkanResourceElement>* resourceDefinition = nullptr;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorPoolSize> poolSizes;
	std::vector<VkWriteDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSetLayoutBinding> textureBindings;
	std::vector<VkDescriptorPoolSize> texturePoolSizes;

	// look for VulkanResourceType in resourceDefinition vector and return index of first occurence, 
	// Error() if not found
	size_t getResourceDefIndex(VulkanResourceType t);
	// add resources for element af specific type.
	// option to overide if more sets needed (e.g. 2x uniform buffers for LineShader)
    // used to define the descriptor set layout
	void addResourcesForElement(VulkanResourceElement el);
    // used to create the actual resources in preparation for vkUpdateDescriptorSets
	void addThreadResourcesForElement(VulkanResourceElement d, VulkanHandoverResources& res);
	// create resources for global texture descriptor set layout, independent of other descriptor sets
	void createDescriptorSetResourcesForTextures();
	//VkDescriptorSetLayout layout = nullptr;
	//VkDescriptorPool pool = nullptr;
	// resources for temporary store info objects between the various create... calls:
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
	//bool globalTextureDescriptorSetValid = false;
	bool addGeomShaderStageToMVP = false;
};
