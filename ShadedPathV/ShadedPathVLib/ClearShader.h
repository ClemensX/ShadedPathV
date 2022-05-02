#pragma once

// ClearShader is used as first shader to clear framebuffer and depth buffers.
// Creates a static command buffer during initialization that can simply be apllied later in Frame drawing
// as first step.
class ClearShader : public ShaderBase
{
public:
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState &shaderState) override;
    virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
    virtual void createCommandBuffer(ThreadResources& tr) override;
    virtual void addCurrentCommandBuffer(ThreadResources& tr) override;

    virtual ~ClearShader() override;

    // create descriptor set layout (one per effect)
    virtual void createDescriptorSetLayout() override;
    // create descritor sets (one or more per render thread)
    virtual void createDescriptorSets(ThreadResources& res) override;

private:
};

