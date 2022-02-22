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
    CameraPositioner_FirstPerson* positioner;
    // mouse pos in device coords: [0..1]
    //glm::vec2 mouseDevicePos = glm::vec2(0.0f);
    InputState input;

};

