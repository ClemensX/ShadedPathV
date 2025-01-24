#pragma once

// simple app only rendering lines with LineShader
class LineApp : ShadedPathApplication, public AppSupport
{
public:
    void mainThreadHook() override;
    void prepareFrame(FrameResources* fi) override;
    void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override;
    void run(ContinuationInfo* cont) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;
    //void init();
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
};

