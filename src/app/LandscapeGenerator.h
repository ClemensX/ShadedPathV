#pragma once

#include "ShadedPathEngine.h"
// generate a landscape using Spatial2D::diamondSquare()
class LandscapeGenerator : ShadedPathApplication, public AppSupport
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
    CameraPositioner_AutoMove* autoMovePositioner;
    InputState input;
    Spatial2D heightmap;
    void writeHeightmapToRawFile();
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
    bool useAutoCameraCheckbox = false;
};
