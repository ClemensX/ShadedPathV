#pragma once

// forward

class DirectImage : public ImageConsumer
{
public:
    DirectImage(ShadedPathEngine* s);
    ~DirectImage();
    void consume(GPUImage* gpui) override;
    void dumpToFile(GPUImage* gpui);
    void dumpToFile(GPUImage* gpui, VkCommandBuffer commandBuffer);
};