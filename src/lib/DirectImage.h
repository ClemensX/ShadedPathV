#pragma once

// forward

class DirectImage : public ImageConsumer
{
public:
    DirectImage(ShadedPathEngine* s);
    DirectImage();
    ~DirectImage();
    void consume(FrameResources* fi) override;
    void dumpToFile(GPUImage* gpui);
    void openForCPUWriteAccess(GPUImage* gpui, GPUImage* writeable);
    void closeCPUWriteAccess(GPUImage* gpui, GPUImage* writeable);
    static void toLayout(VkImageLayout layout, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkCommandBuffer commandBuffer, GPUImage* gpui);
    static void toLayout(VkImageLayout layout, VkAccessFlags2 access, VkCommandBuffer commandBuffer, GPUImage* gpui);
    static void toLayoutAllStagesOnlyForDebugging(VkImageLayout layout, VkCommandBuffer commandBuffer, GPUImage* gpui);
    static void copyBackbufferImageP(GPUImage* gpui_source, GPUImage* gpui_target, VkCommandBuffer commandBuffer, ShadedPathEngine* engine);
private:
    void copyBackbufferImage(GPUImage* gpui_source, GPUImage* gpui_target, VkCommandBuffer commandBuffer);
    unsigned int imageCounter = 0;
};