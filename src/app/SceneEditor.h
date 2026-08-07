#pragma once

struct SceneEditorDisplayParams
{
    bool showGrid = true;
    bool showBoundingBoxes = false;
    bool showMeshletBoundingBoxes = false;
    bool showNormals = false;
    bool showWireframe = false;
    bool showSunDirBeam = false;
    float sunIntensity = 1.0f;

    bool showEnvCubeDialog = false;
};

// Display Logo. May be used while big game loads in the background
class SceneEditor : public ShadedPathApplication, public AppSupport
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
    void buildCustomUI() override;
private:
    World world;
    bool shouldStopEngine = false;
    SceneEditorDisplayParams displayParams;
};