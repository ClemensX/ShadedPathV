#pragma once
// render some lines in device coordinates to visualize Vulkan Normalized Device Coordinates (NDC Coordinates)
class DeviceCoordApp : ShadedPathApplication, public AppSupport
{
public:
    void init();
    void run(ContinuationInfo* cont) override;
    // prepare drawing, guaranteed single thread
    void prepareFrame(FrameResources* fi) override;
    // draw from multiple threads
    void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) override;
    // present or dump to file
    void postFrame(FrameResources* fi) override;
    // process finished frame
    // process finished frame
    void processImage(FrameResources* fi) override;
    void handleInput(InputState& inputState) override;
    bool shouldClose() override;
private:
    Camera* camera;
    CameraPositioner_FirstPerson* positioner = nullptr;
    CameraPositioner_FirstPerson positioner_;
    // mouse pos in device coords: [0..1]
    //glm::vec2 mouseDevicePos = glm::vec2(0.0f);
    InputState input;
    World world;
    bool shouldStopEngine = false;
};

