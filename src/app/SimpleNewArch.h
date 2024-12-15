#pragma once
class SimpleApp : public ShadedPathApplication
{
public:
    void prepareFrame(FrameInfo* fi) override;
    GPUImage* drawFrame(FrameInfo* fi) override;
    void run() override;
    bool shouldClose() override;

    long lastFrameNum = 0;
private:
    bool shouldStop = false;
    DirectImage di;
    GPUImage* gpui = nullptr;
    GPUImage directImage;

    //    void drawFrame(ThreadResources& tr) override {
    //        engine->shaders.submitFrame(tr);
    //    };
    //    void handleInput(InputState& inputState) override {
    //    };
};
