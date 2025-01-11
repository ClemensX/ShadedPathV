#pragma once
class SimpleMultiWin : public ShadedPathApplication
{
public:
    void mainThreadHook() override;
    void prepareFrame(FrameInfo* fi) override;
    void drawFrame(FrameInfo* fi, int topic) override;
    void run(ContinuationInfo* cont = nullptr) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;

    long lastFrameNum = 0;
private:
    bool shouldStopEngine = false;
    DirectImage di;
    GPUImage* gpui = nullptr;
    GPUImage directImage;
    void openWindow(const char* title);
    void openAnotherWindow(const char* title);
    WindowInfo window1;
    WindowInfo window2;
    bool window1wasopened = false;
    bool window2wasopened = false;
    ImageConsumerWindow *imageConsumer = nullptr;
    ImageConsumerNullify imageConsumerNullify;

    //    void drawFrame(ThreadResources& tr) override {
    //        engine->shaders.submitFrame(tr);
    //    };
    //    void handleInput(InputState& inputState) override {
    //    };
};
