#pragma once
class SimpleApp : public ShadedPathApplication
{
public:
    void mainThreadHook() override;
    void prepareFrame(FrameInfo* fi) override;
    void drawFrame(FrameInfo* fi, int topic) override;
    void run() override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;

    long lastFrameNum = 0;
private:
    bool shouldStop = false;
    DirectImage di;
    GPUImage* gpui = nullptr;
    GPUImage directImage;
    void openWindow(const char* title);
    void openAnotherWindow(const char* title);
    WindowInfo window1;
    WindowInfo window2;
    ImageConsumerWindow *imageConsumer = nullptr;

    //    void drawFrame(ThreadResources& tr) override {
    //        engine->shaders.submitFrame(tr);
    //    };
    //    void handleInput(InputState& inputState) override {
    //    };
};
