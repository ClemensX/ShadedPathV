#pragma once

struct ObjectParams
{
    std::string name;
    std::string meshFile;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};
struct SceneEditorDisplayParams
{
    bool showGrid = true;
    bool showBoundingBoxes = false;
    bool showMeshletBoundingBoxes = false;
    bool showNormals = false;
    bool showWireframe = false;
    bool showSunDirBeam = false;
    bool addFixedObjectToScene = false;
    float sunIntensity = 1.0f;
    bool showFileDialog = false;
    bool showSunBeams = false;
    bool sunBeamsInitialized = false;

    // scene handling:
    std::string sceneFileName = "scene_editor_scene.json";
    bool loadSceneRequested = false;
    bool saveSceneRequested = false;

    // object adding:
    bool addStationaryObjectToScene = false;
    bool addMovingObjectToScene = false;
    int selectedLoadedMeshFileLine = -1;

    // environment cube handling:
    bool showEnvCubeDialog = false;
    std::vector<std::string> files;
    std::string filePattern = "";
    int selectedLine = -1;
    // results:
    bool loadNewEnvCube = false; // signal to load new file
    std::string newEnvCubeFileName; // name of new file to load

    // mesh file dialog handling:
    bool showAddMeshFileDialog = false;
    bool loadNewMeshFile = false;
    std::string newMeshFileName;
    bool fileDialogLoadedNewMeshFileDuringSession = false;
    bool fileDialogSelectedObjectForAddingDuringSession = false;
    bool fileDialogWasOpen = false;
    std::vector<GPUModel> stationaryModels;
    std::vector<GPUModel> movingModels;

    // stationary objects re-upload:
    bool reuploadStationaryObjects = false;
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
    void loadNewEnvCube(std::string textureFileName);
    GPUFrameParam frameParam;
    //void addObjectToScene(const ObjectParams& params);
    void addObjectToScene(int meshFileIndex, bool moving);
    bool showFileDialog(bool& loadedNewMeshFile, bool& selectedObjectForAdding, bool showAddObjectButton);
    void loadNewMeshFile(std::string meshFilePathName);
    void fillStationaryModels();
    void fillMovingModels();
    void redoAllStationaryObjects();
    void redoAllMovingObjects();
    void initSunRays();
    PBRShader::LightSource ls;
    // after changing meshes, objects or env cube we must re-init the PBR graphics.
    // This is NOT something you do in regular gameplay, because it will pause frame rendering until all is updated
    void reInitPBRGraphics(bool redoCommandBuffers);

    // scene handling:
    void saveSceneToFile(const std::string& sceneFilePathName);
    void loadSceneFromFile(const std::string& sceneFilePathName);

    // --- Sun ray data ---
    std::vector<LineDef> sunRays;
    glm::vec3 sunRayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec4 sunRayColor = glm::vec4(1.0f);
    float sunRayLength = 25.0f;
    float sunRaySpeed = 8.0f;
    BoundingBox sunRayBox{};
    // --- Sun ray simulation ---
    void setupSunRays(const glm::vec3& sunDirection, int numberRays, float rayLength, float raySpeed, const glm::vec4& rayColor);
    void advanceSunRays(float deltaSeconds, const BoundingBox& simulationBox);
    LineDef createRandomSunRay(const BoundingBox& box) const;
    static bool isInsideBox(const glm::vec3& p, const BoundingBox& box);
};