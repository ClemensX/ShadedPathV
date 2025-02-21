#pragma once

#include "ShadedPathEngine.h"
// generate a landscape using Spatial2D::diamondSquare()
class LandscapeGenerator : ShadedPathApplication, public AppSupport
{
public:
    void run(ContinuationInfo* cont) override;
    // called from main thread
    void init();
    void mainThreadHook() override;
    // prepare drawing, guaranteed single thread
    void prepareFrame(FrameResources* fi) override;
    // draw from multiple threads
    void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override;
    // present or dump to file
    void postFrame(FrameResources* fi) override;
    // process finished frame
    void processImage(FrameResources* fi) override;
    bool shouldClose() override;
    void handleInput(InputState& inputState) override;
    void backgroundWork() override;
    void buildCustomUI() override;
private:
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
    std::string helpText =
        "g generate new seed\n"
        "+ next Generation\n"
        "- previous Generation\n"
        "h write heightmap to file (VK_FORMAT_R32_SFLOAT)\n"
        "p dump image";
    Parameters localp = initialParameters;
    bool shouldStopEngine = false;
    long frameNum = 0;
    int updatesPending = 0;
};
