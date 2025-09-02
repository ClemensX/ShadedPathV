#pragma once

// Display Logo. May be used while big game loads in the background
class MeshManager : ShadedPathApplication, public AppSupport
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
    WorldObject *object = nullptr;
    float plus = 0.0f;
    bool shouldStopEngine = false;
    bool enableUI = true;
    bool spinningBox = false; // do not change - time dependent value
    double spinTimeSeconds = 0.0;
    bool alterObjectCoords = false; // some gltf examples objects require different object rotation params
    bool firstPersonMode = true;
    bool doRotation = false;
    bool useDefaultNormalLineLength = true; // to use different normal line length for some models
    bool loadNewFile = false; // signal to load new file, activated by ImGui
    std::string newFileName; // name of new file to load
    int loadObjectNum = 0; // used to generate new object IDs when loading new objects
    bool planeGrid = false;
    int gridSpacing = 1; // m between grid lines
    std::vector<LineDef> grid1;
    std::vector<LineDef>  grid10;
    std::vector<LineDef>  grid100;
    int uiCameraSpeed = 0; // set by UI
    int oldUiCameraSpeed = 0; // to detect changes
    bool displayNoMeshletDataWarning = false;
};