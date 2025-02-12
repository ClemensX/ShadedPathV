#pragma once

// forward
class UISubShader;

// UIShader is used to render Dear ImGui UI.
class UIShader : public ShaderBase
{
public:
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
    virtual void finishInitialization(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void createCommandBuffer(FrameResources& tr) override;
    virtual void addCurrentCommandBuffer(FrameResources& tr) override;
    virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;
    virtual void destroyThreadResources(FrameResources& tr) override;
    std::vector<UISubShader> perFrameSubShaders;


    // render UI, only to be called from Presentation::presentBackBufferImage() because ImGUI is not thread save
    void draw(FrameResources* fr, WindowInfo* winfo, GPUImage* srcImage);

    virtual ~UIShader() override;

private:
};

class UISubShader
{
public:
    // name is used in shader debugging
    void init(UIShader* parent, std::string debugName);
    void initSingle(FrameResources& tr, ShaderState& shaderState);
    void allocateCommandBuffer(FrameResources& tr, const char* debugName);
    void destroy();

    VkFramebuffer framebuffer = nullptr;
    VkCommandBuffer commandBuffer = nullptr;

private:
    UIShader* uiShader = nullptr;
    std::string name;
    ShadedPathEngine* engine = nullptr;
    VkDevice device = nullptr;
};