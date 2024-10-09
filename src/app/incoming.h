#pragma once

// example for loading and rendering gltf models
class Incoming : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
    void addRandomHeightLines(std::vector<LineDef>& lines, World& world);

private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    WorldObject *worldObject = nullptr;
    unsigned long uiVerticesTotal = 0;
    unsigned long uiVerticesSqrt = 0;
};

