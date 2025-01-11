#pragma once
class SimpleMultiApp2 : public ShadedPathApplication
{
public:
    void mainThreadHook() override;
    void prepareFrame(FrameInfo* fi) override;
    void drawFrame(FrameInfo* fi, int topic) override;
    void run(ContinuationInfo* cont) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;

    long lastFrameNum = 0;
private:
    bool shouldStopEngine = false;
    DirectImage di;
    GPUImage* gpui = nullptr;
    GPUImage directImage;
    void openWindow(const char* title);
    WindowInfo window1;
    bool window1wasopened = false;
    ImageConsumerWindow *imageConsumer = nullptr;
    ImageConsumerNullify imageConsumerNullify;
};

class SimpleMultiApp : public ShadedPathApplication
{
public:
    void mainThreadHook() override;
    void prepareFrame(FrameInfo* fi) override;
    void drawFrame(FrameInfo* fi, int topic) override;
    void run(ContinuationInfo* cont) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;

    long lastFrameNum = 0;
private:
    bool shouldStopEngine = false;
    bool shouldStopAllApplications = false;
    DirectImage di;
    GPUImage* gpui = nullptr;
    GPUImage directImage;
    void openWindow(const char* title);
    WindowInfo window1;
    bool window1wasopened = false;
    ImageConsumerWindow* imageConsumer = nullptr;
    ImageConsumerNullify imageConsumerNullify;
};
