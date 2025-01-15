#pragma once

// Use this example with care. It is a basic test for using mutiple windows with one app.
// Currently produces validation warnings if a window is removed from render queue.
// It is not recommended to use this as a base for your application.
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
    void reuseWindow(const char* title);
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

int mainSimpleMultiWin();
