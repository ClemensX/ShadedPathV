#pragma once

// Display Logo. May be used while big game loads in the background
class Loader : ShadedPathApplication, public AppSupport
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
    // test meshlet by coloring base mesh:
    void debugColors(std::string meshName);
private:
    World world;
    WorldObject *object = nullptr;
    float plus = 0.0f;
    bool shouldStopEngine = false;
    bool enableUI = true;
    bool spinningBox = false; // do not change - time dependent value
    double spinTimeSeconds = 0.0;
    bool alterObjectCoords = false; // some gltf examples objects require different object rotation params
    bool firstPersonMode = true;
    bool doRotation = false;
};

