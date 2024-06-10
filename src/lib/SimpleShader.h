#pragma once
struct SimpleThreadResources : ShaderThreadResources {
    VkPipelineLayout pipelineLayout = nullptr;
    VkPipeline graphicsPipeline = nullptr;
    VkRenderPass renderPass = nullptr;
    VkCommandBuffer commandBuffer = nullptr;
    VkBuffer uniformBuffer = nullptr;
    VkDeviceMemory uniformBufferMemory = nullptr;
    VkDescriptorSet descriptorSet = nullptr;
    VkFramebuffer framebuffer = nullptr;
    VkFramebuffer framebuffer2 = nullptr;
};

class SimpleShader : public ShaderBase
{
public:
    std::vector<VulkanResourceElement> vulkanResourceDefinition = {
        { VulkanResourceType::MVPBuffer },
        { VulkanResourceType::SingleTexture },
        { VulkanResourceType::GlobalTextureSet },
        { VulkanResourceType::VertexBufferStatic },
        { VulkanResourceType::IndexBufferStatic }
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
    };

    const std::vector<Vertex> vertices = {
        //{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        //{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        //{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        //{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -11.1f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, -10.1f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -10.1f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, -10.1f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},


        { {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4

    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // get static std::array of attribute desciptions, make sure to copy to local array, otherwise you get dangling pointers!
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        // layout(location = 0) in vec3 inPosition;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // layout(location = 1) in vec3 inColor;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        // layout(location = 2) in vec2 inTexCoord;
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }


    // update per frame data
    void uploadToGPU(ThreadResources& tr, UniformBufferObject& ubo);
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
    virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
    virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
    // create command buffers. One time auto called before rendering starts.
    // Also post init phase stuff goes here, like VulcanResources.updateDescriptorSets()
    virtual void createCommandBuffer(ThreadResources& tr) override;
    virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
    virtual void destroyThreadResources(ThreadResources& tr) override;


    virtual ~SimpleShader() override;

    // pre-record draw commands (one time call)
    void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, VkBuffer indexBuffer);
    
    // create descriptor set layout (one per effect)
    virtual void createDescriptorSetLayout() override;
    // create descritor sets (one or more per render thread)
    virtual void createDescriptorSets(ThreadResources& res) override;

    TextureInfo* texture = nullptr;

private:
    VkShaderModule vertShaderModuleTriangle = nullptr;
    VkShaderModule fragShaderModuleTriangle = nullptr;
    VkBuffer vertexBufferTriangle = nullptr;
    VkDeviceMemory vertexBufferMemoryTriangle = nullptr;
    VkBuffer indexBufferTriangle = nullptr;
    VkDeviceMemory indexBufferMemoryTriangle = nullptr;
};

