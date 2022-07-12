#pragma once

// example for loading and rendering gltf models
class gltfObjectsApp : ShadedPathApplication
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
private:
    bool enableLines = false;
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    Camera* camera;
    CameraPositioner_FirstPerson* positioner;
    InputState input;
    World world;
    WorldObject *knife1, *knife2;
};

