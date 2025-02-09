#pragma once

// simple app only rendering lines with LineShader
class LineApp : ShadedPathApplication, public AppSupport
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
    //void run();
    //void drawFrame(ThreadResources& tr) override;
    //void handleInput(InputState& inputState) override;
private:
    //void updatePerFrame(ThreadResources& tr);

    // implement square formed of lines
    // it is increased on every call by one more square slightly above the others
    void increaseLineStack(std::vector<LineDef>& lines);
    int currentLineStackCount = 0;
    float plus = 0.0f;
    bool shouldStopEngine = false;
    long frameNum = 0;
};

