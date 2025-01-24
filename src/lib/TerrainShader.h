#pragma once
// TODO: currently not in use!!!!
// TerrainShader - draw terrain using heightmaps and textures
// stages: VertexShader --> TessellationControlShader (optional) --> TessellationEvaluationShader (optional) --> GeometryShader (optional) --> FragmentShader
// This is a basic structure. Implement tessellation or geometry shaders as needed for your terrain rendering technique.

// TerrainVertex is used in application code to define the terrain mesh AND directly used as Vertex definition
struct TerrainVertex {
    glm::vec3 pos; // Position of the vertex
    glm::vec3 normal; // Normal vector for lighting
    glm::vec2 texCoords; // Texture coordinates
};

// Uniform buffer object for terrain shader
struct TerrainUniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class TerrainShader : public ShaderBase {
public:
    std::vector<VulkanResourceElement> vulkanResourceDefinition = {
        { VulkanResourceType::MVPBuffer },
        { VulkanResourceType::GlobalTextureSet }, // Assuming terrain uses a global texture set
        { VulkanResourceType::VertexBufferStatic } // Static vertex buffer for terrain mesh
    };

    // Define terrain vertex layout
    typedef TerrainVertex Vertex;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    // Get static std::array of attribute descriptions
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        // layout(location = 0) in vec3 inPosition;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // layout(location = 1) in vec3 inNormal;
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        // layout(location = 2) in vec2 inTexCoords;
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoords);
        return attributeDescriptions;
    }

    virtual ~TerrainShader() override;
    virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
    virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
    virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void createCommandBuffer(FrameResources& tr) override;
    virtual void addCurrentCommandBuffer(FrameResources& tr) override;
    virtual void destroyThreadResources(FrameResources& tr) override;

    // Additional methods specific to terrain rendering can be added here

private:
    void recordDrawCommand(VkCommandBuffer& commandBuffer, ThreadResources& tr, VkBuffer vertexBuffer, bool isRightEye = false);

    TerrainUniformBufferObject ubo = {};
    bool disabled = false;

    // Vertex buffer for terrain mesh (one buffer for all threads)
    VkBuffer vertexBuffer = nullptr;
    VkDeviceMemory vertexBufferMemory = nullptr;
    VkShaderModule vertShaderModule = nullptr;
    VkShaderModule fragShaderModule = nullptr;
    // Add tessellation and geometry shader modules as needed
};

struct TerrainThreadResources : ShaderThreadResources {
    // Similar to BillboardThreadResources, adjust as necessary for terrain rendering
    VkFramebuffer framebuffer = nullptr;
    VkRenderPass renderPass = nullptr;
    VkPipelineLayout pipelineLayout = nullptr;
    VkPipeline graphicsPipeline = nullptr;
    VkCommandBuffer commandBuffer = nullptr;
    VkBuffer uniformBuffer = nullptr;
    VkDeviceMemory uniformBufferMemory = nullptr;
    VkDescriptorSet descriptorSet = nullptr;
};
