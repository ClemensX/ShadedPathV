#pragma once

// generate billboards to teview mass rendering
class LandscapeGenerator : ShadedPathApplication
{
public:
    void init();
    void run();
    void drawFrame(ThreadResources& tr) override;
    void handleInput(InputState& inputState) override;
    void buildCustomUI() override;
private:
    ShadedPathEngine engine;
    Shaders& shaders = engine.shaders;
    void updatePerFrame(ThreadResources& tr);
    Camera* camera;
    CameraPositioner_FirstPerson* positioner;
    InputState input;
    World world;
    Spatial2D* heightmap = nullptr;
    // generation parameters set from UI:
    struct Parameters {
        int n;
        float dampening;
        float magnitude;
        int seed;
        float h_tl;
        float h_tr;
        float h_bl;
        float h_br;
        int generations;
        bool generate = false;
    };
    Parameters parameters;
    // store generated lines:
    std::vector<LineDef> lines;
    std::mutex monitorMutex;
};
