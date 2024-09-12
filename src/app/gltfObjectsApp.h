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
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    World world;
    WorldObject *bottle = nullptr;
};

