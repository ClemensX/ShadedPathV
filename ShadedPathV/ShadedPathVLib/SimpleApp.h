#pragma once
class SimpleApp : ShadedPathApplication
{
public:
    void run();
    void drawFrame(ThreadResources& tr);
    void handleInput(InputState& inputState);
private:
    ShadedPathEngine engine;
    void updatePerFrame(ThreadResources& tr);
    Camera* camera;
};

