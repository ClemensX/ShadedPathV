#pragma once

// forward

class DirectImage : public ImageConsumer
{
public:
    DirectImage(ShadedPathEngine* s);
    DirectImage();
    ~DirectImage();
    void consume(GPUImage* gpui) override;
    void dumpToFile(GPUImage* gpui);
    void openForCPUWriteAccess(GPUImage* gpui, GPUImage* writeable);
    void closeCPUWriteAccess(GPUImage* gpui, GPUImage* writeable);
    void toLayout(VkImageLayout layout, VkAccessFlags access, VkCommandBuffer commandBuffer, GPUImage* gpui);
private:
    void copyBackbufferImage(GPUImage* gpui_source, GPUImage* gpui_target, VkCommandBuffer commandBuffer);
    unsigned int imageCounter = 0;
};