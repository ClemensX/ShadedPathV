#pragma once

// generate billboards to teview mass rendering
class BillboardDemo : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void addRandomBillboards(std::vector<BillboardDef>& billboards, World& world, unsigned int textureIndex, float aspectRatio);
private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
};
