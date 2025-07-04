#pragma once

// forward
class UIShader;

// UIShader is used to render Dear ImGui UI.
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

class UIShader : public ShaderBase
{
public:
    // set up shader
    virtual void init(ShadedPathEngine& engine, ShaderState& shaderState) override;
    virtual void initSingle(FrameResources& tr, ShaderState& shaderState) override;
    virtual void createCommandBuffer(FrameResources& tr) override;
    virtual void addCommandBuffers(FrameResources* fr, DrawResult* drawResult) override;
    //std::vector<UISubShader> perFrameSubShaders;
    UISubShader subShader; // single subshader for UI, as it is not used in stereo or vr mode


    // render UI, only to be called from Presentation::presentBackBufferImage() because ImGUI is not thread save
    // will only render for left eye in stereo or vr mode
    void draw(FrameResources* fr, WindowInfo* winfo, GPUImage* srcImage);
    void initFramebuffer(VkImageView view, VkFramebuffer& framebuffer);
    virtual ~UIShader() override;

private:
};
