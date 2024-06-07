#pragma once

#include "ShadedPathEngine.h"
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
        bool paramsChangedOutsideUI = false;
    };
    // define set of initial parameters - used here and in Imgui init code
    const Parameters initialParameters = { 10, 0.6f, 200.0f, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1 };
    Parameters parameters = initialParameters;
    // store generated lines:
    std::vector<LineDef> lines;
    std::mutex monitorMutex;
    bool useAutoCamera = false;
};
