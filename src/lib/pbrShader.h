#pragma once

struct MeshInfo;
class WorldObject;

// forward
class PBRSubShader;

// pbr shader draws objects read from glTF files with PBR lighing
class PBRShader : public ShaderBase {
public:
	// We have to set max number of objects, as dynamic uniform buffers have to be allocated (one entry for each object in a large buffer)
	uint64_t MaxObjects = 1000;

	std::vector<PBRSubShader> globalSubShaders;

	std::vector<VulkanResourceElement> vulkanResourceDefinition = {
		{ VulkanResourceType::MVPBuffer },
		{ VulkanResourceType::UniformBufferDynamic },
		{ VulkanResourceType::GlobalTextureSet },
		{ VulkanResourceType::VertexBufferStatic },
		{ VulkanResourceType::MeshShader }
	};

	struct Vertex {
		glm::vec3 pos; // 12 bytes
		float pad0;
        glm::vec3 normal; // 12 bytes
		float pad1;
        glm::vec2 uv0; // 8 bytes
        glm::vec2 uv1; // 8 bytes
        glm::uvec4 joint0; // 16 bytes, 4 joints per vertex
        glm::vec4 weight0; // 16 bytes, 4 weights per vertex
        glm::vec4 color; // 16 bytes, color in vertex structure
        //uint32_t pad[2]; // 8 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!

        // total size: 12 + 12 + 8 + 8 + 16 + 16 + 16 = 88 bytes

		bool operator==(const Vertex& other) const {
			return pos == other.pos &&
				normal == other.normal &&
				uv0 == other.uv0 &&
				uv1 == other.uv1 &&
				joint0 == other.joint0 &&
				weight0 == other.weight0 &&
				color == other.color;
		}
    };

	struct shaderValuesParams {
		glm::vec4 lightDir;
		float exposure = 4.5f;
		float gamma = 2.2f;
		float prefilteredCubeMipLevels;
		float scaleIBLAmbient = 1.0f;
		float debugViewInputs = 0;
		float debugViewEquation = 0;
		float pad0;
		float pad1;
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 baseColor = glm::vec4(1.0f);
        glm::vec3 camPos = glm::vec3(-42.0f); // signal that this is not set
	};

	// MUST match shader definition: pbr.vert, pbr.frag
    // DO NOT USE arrays for padding on glsl side! only single variables like uint pad0, uint pad1, ...
#define MAX_NUM_JOINTS 128
	struct PBRTextureIndexes {
		uint32_t baseColor; // uint in shader
		uint32_t metallicRoughness; // uint in shader
		uint32_t normal; // uint in shader
		uint32_t occlusion; // uint in shader
		uint32_t emissive; // uint in shader
		uint32_t pad0[3]; // 12 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!
	};
	struct TexCoordSets {
		uint32_t baseColor = 0;
		uint32_t metallicRoughness = 0;
		uint32_t specularGlossiness = 0;
		uint32_t normal = 0;
		uint32_t occlusion = 0;
		uint32_t emissive = 0;
		uint32_t pad0[2]; // 8 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!
	};

	struct ShaderMaterial {
		glm::vec4 baseColorFactor;
		glm::vec4 emissiveFactor;
		glm::vec4 diffuseFactor;
		glm::vec4 specularFactor;
		float workflow;
		int baseColorTextureSet;
		int physicalDescriptorTextureSet;
		int normalTextureSet;
		int occlusionTextureSet;
		int emissiveTextureSet;
		int brdflut;
		int irradiance;
		int envcube;
		float metallicFactor;
		float roughnessFactor;
		float alphaMask;
		float alphaMaskCutoff;
		float emissiveStrength;
		uint32_t pad0;
		uint32_t pad1;
		TexCoordSets texCoordSets;
	};
    // the dynamic uniform buffer is peramnently mapped to CPU memory for fast updates
	struct alignas(16) DynamicModelUBO {
		glm::mat4 model; // 16-byte aligned
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]; // 16-byte aligned
		uint32_t jointcount; // 4-byte aligned
		uint32_t flags = 0; // 4-byte aligned
		uint32_t meshletsCount = 0; // 4-byte aligned
		uint32_t mode; // 4 bytes mode: 1 == use vertex color only, 0 == regular BPR rendering
		PBRTextureIndexes indexes; // 4-byte aligned
        shaderValuesParams params; // 16-byte aligned
        ShaderMaterial material; // 16-byte aligned
        BoundingBox boundingBox; // AABB in local object space
	};
	// Array entries of DynamicModelUBO have to respect hardware alignment rules
	uint64_t alignedDynamicUniformBufferSize = 0;

	// MeshletOld descriptor, from NVIDIA Descriptor B in https://jcgt.org/published/0012/02/01/
	// 128 bits = 16 bytes
	struct alignas(16) PackedMeshletDesc {
		uint8_t data[16];

		// Packing function
		// vertexPack == 0 means default rendering, 1 means use debug meshlet colors
		static PackedMeshletDesc pack(
			uint64_t boundingBox,      // 48 bits
			uint8_t numVertices,       // 8 bits
			uint8_t numPrimitives,     // 8 bits
			uint8_t vertexPack,        // 8 bits
			uint32_t indexBufferOffset,// 32 bits
			uint32_t normalCone        // 24 bits (store in lower 24 bits)
		) {
			PackedMeshletDesc p{};
			uint64_t* u64 = reinterpret_cast<uint64_t*>(p.data);
			uint64_t low = 0, high = 0;

			// Layout:
			// | boundingBox (48) | numVertices (8) | numPrimitives (8) | vertexPack (8) | indexBufferOffset (32) | normalCone (24) |
			// [0:47]             [48:55]           [56:63]             [64:71]         [72:103]                [104:127]

			// Pack lower 64 bits
			low |= (boundingBox & 0xFFFFFFFFFFFFULL);           // 48 bits
			low |= (uint64_t(numVertices) & 0xFF) << 48;        // 8 bits
			low |= (uint64_t(numPrimitives) & 0xFF) << 56;      // 8 bits

			// Pack upper 64 bits
			high |= (uint64_t(vertexPack) & 0xFF);              // 8 bits
			high |= (uint64_t(indexBufferOffset) & 0xFFFFFFFF) << 8; // 32 bits
			high |= (uint64_t(normalCone) & 0xFFFFFF) << 40;    // 24 bits

			u64[0] = low;
			u64[1] = high;
			return p;
		}

		// Unpacking functions
		uint64_t getBoundingBox() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return u64[0] & 0xFFFFFFFFFFFFULL;
		}
		uint8_t getNumVertices() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return (u64[0] >> 48) & 0xFF;
		}
		uint8_t getNumPrimitives() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return (u64[0] >> 56) & 0xFF;
		}
        // 0 means default rendering, 1 means use debug meshlet colors
		uint8_t getVertexPack() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return u64[1] & 0xFF;
		}
		uint32_t getIndexBufferOffset() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return (u64[1] >> 8) & 0xFFFFFFFF;
		}
		uint32_t getNormalCone() const {
			const uint64_t* u64 = reinterpret_cast<const uint64_t*>(data);
			return (u64[1] >> 40) & 0xFFFFFF;
		}
	};

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	// get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
	static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};

		// layout(location = 0) in vec3 inPos;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		// layout(location = 1) in vec3 inNormal;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		// layout(location = 2) in vec2 inUV0;
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, uv0);

		// layout(location = 3) in vec2 inUV1;
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, uv1);

		// layout(location = 4) in uvec4 inJoint0;
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_UINT;
		attributeDescriptions[4].offset = offsetof(Vertex, joint0);

		// layout(location = 5) in vec4 inWeight0;
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(Vertex, weight0);

		// layout(location = 6) in vec4 inColor0;
		attributeDescriptions[6].binding = 0;
		attributeDescriptions[6].location = 6;
		attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[6].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	virtual ~PBRShader() override;

	// shader initialization, end result is a shader sub resource for each worker thread
	virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
	// frame resources initialization
	virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
	// create command buffers. One time auto called before rendering starts
	// we create one command buffer for every mesh loaded
	virtual void createCommandBuffer(FrameResources& tr) override;
	// add the pre-computed command buffer for the current object
	virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;

	// get access to dynamic uniform buffer for an object
	DynamicModelUBO* getAccessToModel(FrameResources& tr, UINT num);
	
	// upload of all objects to GPU - only valid before first render
	void initialUpload();

	// per frame update of UBOs / MVPs
	void uploadToGPU(FrameResources& tr, UniformBufferObject& ubo, UniformBufferObject& ubo2); // TODO automate handling of 2nd UBO
	// one-time prefill PBR parameters in the dynamic Uniform Buffer.
    // Called once before rendering starts. Apps can change settings anytime by accessing the dynamic buffer via getAccessToModel()
	void prefillModelParameters(FrameResources& tr);

	void fillTextureIndexesFromMesh(PBRTextureIndexes& ind, MeshInfo* mesh);
    void changeLightSource(glm::vec3 color, glm::vec3 rotation) {
		if (commandBuffersCreated) Error("cannot change light source after command buffers have been created. Change for each model in app code!");
        lightSource.color = color;
        lightSource.rotation = rotation;
    }

private:
	UniformBufferObject ubo = {};
	UniformBufferObject updatedUBO = {};
	bool disabled = false;

	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	VkShaderModule taskShaderModule = nullptr;
	VkShaderModule meshShaderModule = nullptr;
	struct LightSource {
		glm::vec3 color = glm::vec3(1.0f);
		glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
	} lightSource;
    bool commandBuffersCreated = false;
};

// Hash combine utility
inline void hash_combine(std::size_t& seed, std::size_t v) {
	seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Specialize std::hash for PBRShader::Vertex
namespace std {
	template<>
	struct hash<PBRShader::Vertex> {
		std::size_t operator()(const PBRShader::Vertex& v) const {
			std::size_t seed = 0;
			hash_combine(seed, std::hash<float>{}(v.pos.x));
			hash_combine(seed, std::hash<float>{}(v.pos.y));
			hash_combine(seed, std::hash<float>{}(v.pos.z));
			hash_combine(seed, std::hash<float>{}(v.normal.x));
			hash_combine(seed, std::hash<float>{}(v.normal.y));
			hash_combine(seed, std::hash<float>{}(v.normal.z));
			hash_combine(seed, std::hash<float>{}(v.uv0.x));
			hash_combine(seed, std::hash<float>{}(v.uv0.y));
			hash_combine(seed, std::hash<float>{}(v.uv1.x));
			hash_combine(seed, std::hash<float>{}(v.uv1.y));
			hash_combine(seed, std::hash<uint32_t>{}(v.joint0.x));
			hash_combine(seed, std::hash<uint32_t>{}(v.joint0.y));
			hash_combine(seed, std::hash<uint32_t>{}(v.joint0.z));
			hash_combine(seed, std::hash<uint32_t>{}(v.joint0.w));
			hash_combine(seed, std::hash<float>{}(v.weight0.x));
			hash_combine(seed, std::hash<float>{}(v.weight0.y));
			hash_combine(seed, std::hash<float>{}(v.weight0.z));
			hash_combine(seed, std::hash<float>{}(v.weight0.w));
			hash_combine(seed, std::hash<float>{}(v.color.x));
			hash_combine(seed, std::hash<float>{}(v.color.y));
			hash_combine(seed, std::hash<float>{}(v.color.z));
			hash_combine(seed, std::hash<float>{}(v.color.w));
			return seed;
		}
	};
}

/*
 * PBRSubShader includes everything for one shader invocation.
 * Currently only 1 sub shader
 */
class PBRSubShader {
public:
	// name is used in shader debugging
	void init(PBRShader* parent, std::string debugName);
	void setVertShaderModule(VkShaderModule sm) {
		vertShaderModule = sm;
	}
	void setFragShaderModule(VkShaderModule sm) {
		fragShaderModule = sm;
	}
	void setTaskShaderModule(VkShaderModule sm) {
		taskShaderModule = sm;
	}
	void setMeshShaderModule(VkShaderModule sm) {
		meshShaderModule = sm;
	}
	void initSingle(FrameResources& tr, ShaderState& shaderState);
	void setVulkanResources(VulkanResources* vr) {
		vulkanResources = vr;
	}

	// All sections need: buffer allocation and recording draw commands.
	// Stage they are called at will be very different
	void allocateCommandBuffer(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName);
	void addRenderPassAndDrawCommands(FrameResources& tr, VkCommandBuffer* cmdBufferPtr, VkBuffer vertexBuffer);

	void createGlobalCommandBufferAndRenderPass(FrameResources& tr);
	void recordDrawCommand(VkCommandBuffer& commandBuffer, FrameResources& tr, WorldObject* obj, bool isRightEye = false);
	// per frame update of UBO / MVP
	void uploadToGPU(FrameResources& tr, PBRShader::UniformBufferObject& ubo, PBRShader::UniformBufferObject& ubo2);

	void destroy();

	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebuffer2 = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	// VP buffer
	VkBuffer uniformBuffer = nullptr;
	VkBuffer uniformBuffer2 = nullptr;
	// Model buffers
	VkBuffer dynamicUniformBuffer = nullptr;
	// VP buffer device memory
	VkDeviceMemory uniformBufferMemory = nullptr;
	VkDeviceMemory uniformBufferMemory2 = nullptr;

	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSet descriptorSet2 = nullptr;
	// M buffer device memory
	VkDeviceMemory dynamicUniformBufferMemory = nullptr;
	void* dynamicUniformBufferCPUMemory = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

private:
	PBRShader* pbrShader = nullptr;
	VulkanResources* vulkanResources = nullptr;
	std::string name;
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
	VkShaderModule taskShaderModule = nullptr;
	VkShaderModule meshShaderModule = nullptr;
	ShadedPathEngine* engine = nullptr;
	VkDevice device = nullptr;
	FrameResources* frameResources = nullptr;
};
