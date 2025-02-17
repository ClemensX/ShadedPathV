#pragma once

// example for loading and rendering gltf models
class gltfTerrainApp : ShadedPathApplication, public AppSupport
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
    WorldObject *worldObject = nullptr;
    unsigned long uiVerticesTotal = 0;
    unsigned long uiVerticesSqrt = 0;
    bool shouldStopEngine = false;
    bool enableLines = true;
    bool enableUI = true;
};

