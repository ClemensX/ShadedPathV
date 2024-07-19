#pragma once

// siple app only rendering lines with LineShader
class LineApp : ShadedPathApplication
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    Camera* camera;
    CameraPositioner_FirstPerson* positioner;
    CameraPositioner_HMD* hmdPositioner;
    InputState input;
    World world;

    // implent square formed of lines
    // it is increased on every call by one more square slightly above the others
    void increaseLineStack(std::vector<LineDef>& lines);
    int currentLineStackCount = 0;
};

