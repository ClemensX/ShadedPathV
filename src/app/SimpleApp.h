#pragma once
class SimpleApp : ShadedPathApplication, public AppSupport
{
public:
    // called from main thread
    void mainThreadHook() override;
    // prepare drawing, guaranteed single thread
    void prepareFrame(FrameResources* fi) override;
    // draw from multiple threads
    void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override;
    // present or dump to file
    void postFrame(FrameResources* fi) override;
    // process finished frame
    void processImage(FrameResources* fi) override;
    void run(ContinuationInfo* cont) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;
    void init();
private:
    bool downmode;
    float plus = 0.0f;
};

