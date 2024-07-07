#pragma once

// example for loading and rendering gltf models
class gltfTerrainApp : ShadedPathApplication
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;

private:
    bool enableLines = false;
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    Camera* camera;
    CameraPositioner_FirstPerson* positioner;
    InputState input;
    World world;
    WorldObject *worldObject = nullptr;
    unsigned long uiVerticesTotal = 0;
    unsigned long uiVerticesSqrt = 0;
};

