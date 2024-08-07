#pragma once

#include "ShaderBase.h"

// forward
class ClearSubShader;

// ClearShader is used as first shader to clear framebuffer and depth buffers.
// Creates a static command buffer during initialization that can simply be applied later in Frame drawing
// as first step.
class ClearShader : public ShaderBase
{
public:
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
    virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void createCommandBuffer(ThreadResources& tr) override;
    virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
    virtual void destroyThreadResources(ThreadResources& tr) override;
    // override default black clear color
    void setClearColor(glm::vec4 color) { clearColor = color; };
    glm::vec4 getClearColor() { return clearColor; };

    virtual ~ClearShader() override;

private:
    std::vector<ClearSubShader> clearSubShaders;
    glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // default black
};

class ClearSubShader
{
public:
    // name is used in shader debugging
    void init(ClearShader* parent, std::string debugName);
    void initSingle(ThreadResources& tr, ShaderState& shaderState) {};
    void allocateCommandBuffer(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr, const char* debugName);
    void createGlobalCommandBufferAndRenderPass(ThreadResources& tr);
    void addRenderPassAndDrawCommands(ThreadResources& tr, VkCommandBuffer* cmdBufferPtr);
    void destroy();
    VkCommandBuffer commandBuffer = nullptr;
    VkFramebuffer framebuffer = nullptr;
    VkFramebuffer framebuffer2 = nullptr;
    VkRenderPass renderPass = nullptr;

private:
    ClearShader* clearShader = nullptr;
    std::string name;
    ShadedPathEngine* engine = nullptr;
    VkDevice* device = nullptr;
};

// use identical shader with new name for ending shader chain
class EndShader : public ClearShader {
    void init(ShadedPathEngine& engine, ShaderState& shaderState) override;
};