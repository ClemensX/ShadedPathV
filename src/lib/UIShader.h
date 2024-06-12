#pragma once

#pragma once
struct UIThreadResources : ShaderThreadResources {
    VkFramebuffer framebuffer = nullptr;
};
// UIShader is used to render Dear ImGui UI.
class UIShader : public ShaderBase
{
public:
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void initSingle(ThreadResources& tr, ShaderState& shaderState) override;
    virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void createCommandBuffer(ThreadResources& tr) override;
    virtual void addCurrentCommandBuffer(ThreadResources& tr) override;
    virtual void destroyThreadResources(ThreadResources& tr) override;


    // render UI, only to be called from Presentation::presentBackBufferImage() because ImGUI is not thread save
    void draw(ThreadResources& tr);

    virtual ~UIShader() override;

private:
};

