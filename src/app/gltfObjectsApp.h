#pragma once

// example for loading and rendering gltf models
class gltfObjectsApp : ShadedPathApplication, public AppSupport
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
    World world;
    WorldObject *bottleX = nullptr;
    WorldObject* grass = nullptr;
    float plus = 0.0f;
    bool shouldStopEngine = false;
    bool enableLines = true;
    bool enableUI = true;
};

