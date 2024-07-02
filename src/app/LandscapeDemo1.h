#pragma once

// generate billboards to teview mass rendering
class LandscapeDemo : ShadedPathApplication
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
    Camera* camera;
    CameraPositioner_FirstPerson* positioner;
    InputState input;
    World world;
    Spatial2D* heightmap = nullptr;
    bool isSkybox = true; // set to false to have center cube instead of skybox
};
