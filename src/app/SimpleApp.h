#pragma once
class SimpleApp : ShadedPathApplication
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
    // mouse pos in device coords: [0..1]
    //glm::vec2 mouseDevicePos = glm::vec2(0.0f);
    InputState input;
    World world;
};

