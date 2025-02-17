#pragma once

// view generated texture projected onto cube that can be viewed from any side
// BRDFLUT (BRDF lookup table)
// Irradiance cube map
// Environment Cube map
class GeneratedTexturesApp : ShadedPathApplication, public AppSupport
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
private:
    void updatePerFrame(ThreadResources& tr);
    //Camera* camera;
    //CameraPositioner_FirstPerson* positioner;
    //InputState input;
    World world;
    WorldObject *knife1, *knife2;
    float plus = 0.0f;
    bool shouldStopEngine = false;
    bool enableLines = false;
};
