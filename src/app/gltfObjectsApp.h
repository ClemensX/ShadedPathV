#pragma once

// example for loading and rendering gltf models
class gltfObjectsApp : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
private:
    void updatePerFrame(ThreadResources& tr);
    World world;
    WorldObject *bottle = nullptr;
    float plus = 0.0f;
};

