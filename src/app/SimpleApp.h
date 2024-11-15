#pragma once
class SimpleApp : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
private:
    void updatePerFrame(ThreadResources& tr);
    bool downmode;
    float plus = 0.0f;
};

