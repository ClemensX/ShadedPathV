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
    WorldObject* object = nullptr; // current object which can be manipulated bu UI
    std::vector<WorldObject*> objects; // all loaded objects (of a collection)
    float plus = 0.0f;
    bool shouldStopEngine = false;
    bool enableUI = true;
    bool spinningBox = false; // do not change - time dependent value
    double spinTimeSeconds = 0.0;
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
    bool regenerateMeshletData = false;
    bool regenerationFinished = false;
    bool showMeshWireframe = false;
    bool showBoundingBox = false;
    bool showMeshletBoundingBoxes = false;
    bool changeAllObjects = true;
    bool applySetupObjects = false; // signal to apply default object setup for LOD checking
    float modelScale = 1.0f;
    glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f); // used as increments to pi/2
    glm::vec3 modelTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
    MeshCollection* meshCollection = nullptr;
    MeshInfo* meshSelectedFromCollection = nullptr;
    bool applyLOD = false; // set objects position according to LOD distances
    float lod[10] = { 0.0f }; // LOD distances, [0] is never used
    float sunIntensity = 1.0f;
    bool addSunDirBeam = false; // add line to point to sun direction, for checking corect sun position
    bool simluateLOD = false;
    float lodDistance = 0.0f; // distance for LOD simulation
    int simLODLevel = 0; // LOD level for simulation
    std::vector<WorldObject*> simObjects; // for simulating LOD
};