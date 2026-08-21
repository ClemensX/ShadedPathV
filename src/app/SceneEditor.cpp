#include "mainheader.h"
#include "AppSupport.h"
#include "SceneEditor.h"

using namespace std;
using namespace glm;

void SceneEditor::run(ContinuationInfo* cont)
{
    Log("SceneEditor started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        engine->appname = "SceneEditor";
        // camera initialization
        //initCamera(vec3(0, 0, 10), vec3(0.0f, 0.0f, -10.0f), vec3(0.0f, 1.0f, 0.0f));
        // camera initialization, like gltf sample viewer
        initCamera(vec3(-0.1f, -0.5f, -2.6f), vec3(0.08f, 0.16f, 0.98f), vec3(0.0f, 1.0f, 0.0f));
        Movement mv;
        //camera->setConstantSpeed(mv.runSpeedMS * 8);
        camera->setConstantSpeed(mv.runSpeedMS);
        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.01f, 5000.0f);

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)
            .addShader(shaders.pbrShader)
            .addShader(shaders.lineShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();
        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("SceneEditor ended" << endl);
}

void SceneEditor::init()
{
    engine->textureStore.generateBRDFLUT();
    {
        // test object
        // use new mstore:
        MeshFlagsCollection flags;
        flags.setFlag(MeshFlags::MESHLET_GENERATE);
        //flags.setFlag(MeshFlags::RENDER_TYPE_MOVING);
        MStore& mstore = engine->mstore;
        //engine->mstore.loadMesh("test/cube_single.gltf", "SingleMesh", flags);
        engine->mstore.loadMesh("DamagedHelmet_cmp.glb", "SingleMesh", flags);
        //engine->mstore.loadMesh("MirrorCube.glb", "SingleMesh", flags);
        auto loaded = mstore.getMeshFileByID("SingleMesh"); // ensure we can retrieve the mesh file by ID
        auto meshInfo = mstore.getGPUMeshInfo(loaded->meshes[0].meshIndex);
        const auto meshMetadata = mstore.getMeshMetadata(loaded->meshes[0].meshIndex);
        SceneObject* object = engine->mstore.addObject(meshInfo->index, vec3(0.0f, 0.0f, 0.0f), flags);
        // turn upside down
        object->rot = vec3(PI_half, PI_half*2, 0.0f);
        object->scale = vec3(1.0f);
        mat4 baseTransform = mat4(1.0); // get from gltf later
        GPUModel* gpuModel = engine->mstore.getGPUModel(object->index);

        object->prepareGPUModel(gpuModel, baseTransform);
    }

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    PBRShader::LightSource ls;
    ls.color = vec3(1.0f);
    ls.position = vec3(75.0f, 0.5f, -20.0f);

    // new
    //engine->shaders.pbrShader.fillStandardFrameParams(frameParam);
    engine->shaders.pbrShader.changeLightSource(frameParam, ls.color, ls.position);
    frameParam.intensity = 1.0f; // adjust sun light intensity
    engine->shaders.pbrShader.setFrameParam(frameParam, 0);

    // old
    engine->shaders.pbrShader.changeLightSource(ls.color, ls.position);
    engine->shaders.pbrShader.initialUpload(true);

    // window creation
    prepareWindowOutput("Scene Editor");
    engine->presentation.startUI();

    // uncomment next block to enable zero cross display
    //LineShader::addZeroCross(lines);

    //engine->shaders.lineShader.addFixedGlobalLines(lines);
    engine->shaders.lineShader.uploadFixedGlobalLines();
}

void SceneEditor::mainThreadHook()
{
}

void SceneEditor::prepareFrame(FrameResources* fr)
{
    FrameResources& tr = *fr;
    double seconds = engine->gameTime.getTimeSeconds();
    if ((old_seconds > 0.0f && old_seconds == seconds) || old_seconds > seconds) {
        Error("APP TIME ERROR - should not happen");
        return;
    }
    double deltaSeconds = seconds - old_seconds;

    Movement mv;
    //camera->setConstantSpeed(mv.fallSpeedMS);
    updateCameraPositioners(deltaSeconds);
    old_seconds = seconds;

    // check for UI actions:
    if (displayParams.loadNewEnvCube) {
        displayParams.loadNewEnvCube = false;
        loadNewEnvCube(displayParams.newEnvCubeFileName);
    }
    if (displayParams.loadNewMeshFile) {
        displayParams.loadNewMeshFile = false;
        loadNewMeshFile(displayParams.newMeshFileName);
    }
    if (displayParams.reuploadStationaryObjects) {
        displayParams.reuploadStationaryObjects = false;
        redoAllStationaryObjects();
    }
    if (displayParams.addFixedObjectToScene) {
        displayParams.addFixedObjectToScene = false;
        Log("Adding fixed object to scene" << endl);
        ObjectParams params;
        params.name = "FixedObject";
        params.meshFile = "path/to/mesh.obj";
        params.position = glm::vec3(0.0f, 0.0f, 0.0f);
        addObjectToScene(params);
    }

    // cube
    CubeShader::UniformBufferObject cubo{};
    CubeShader::UniformBufferObject cubo2{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(cubo.view, cubo.proj, cubo2.view, cubo2.proj);
    // reset view matrix to camera orientation without using camera position (prevent camera movin out of skybox)
    cubo.view = camera->getViewMatrixAtCameraPos();
    cubo2.view = camera->getViewMatrixAtCameraPos();
    engine->shaders.cubeShader.uploadToGPU(tr, cubo, cubo2);

    // pbr
    PBRShader::UniformBufferObject pubo{};
    PBRShader::UniformBufferObject pubo2{};
    mat4 modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    pubo.model = modeltransform;
    pubo2.model = modeltransform;
    //pubo.baseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // be sure to add cam pos to UBO for PBR shader!!!
    applyViewProjection(pubo.view, pubo.proj, pubo2.view, pubo2.proj, &pubo.camPos, &pubo2.camPos);
    //Log("Camera position: " << pubo.camPos.x << " " << pubo.camPos.y << " " << pubo.camPos.z << endl); // Camera position: -0.0386716 0.2 0.51695
    engine->shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);

    postUpdatePerFrame(tr);
    //camera->log();
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void SceneEditor::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
    }
}

void SceneEditor::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void SceneEditor::processImage(FrameResources* fr)
{
    present(fr);
}

bool SceneEditor::shouldClose()
{
    return shouldStopEngine;
}

void SceneEditor::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    auto key = inputState.key;
    auto action = inputState.action;
    auto mods = inputState.mods;
    // spacebar to stop animation
    if (inputState.keyEvent) {
        if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
            //doRotation = !doRotation;
        }
    }
    AppSupport::handleInput(inputState);
}

void SceneEditor::buildCustomUI() {
    ImGui::Separator();
    if (ImGui::Button("Environment Cube Settings")) {
        displayParams.showEnvCubeDialog = true;
        ImGui::OpenPopup("EnvCubeSettings");
    }
    if (ImGui::BeginPopupModal("EnvCubeSettings", &displayParams.showEnvCubeDialog)) {
        // load file list from data folder
        engine->files.findAssetFolder("data"); // maybe let the user change asset folder name?
        filesystem::path textureFolder = engine->files.getAssetFolderPath() / engine->files.TEXTURE_PATH;
        //displayParams.filePattern = ".ktx2";
        displayParams.filePattern = "";
        displayParams.files = Util::getFilesMatchingPattern(textureFolder, displayParams.filePattern);
        ImGui::Text("Data folder: %s", textureFolder.string().c_str());
        ImGui::Text("Choose a file:");
        ImGui::Separator();

        for (int i = 0; i < displayParams.files.size(); ++i) {
            if (ImGui::Selectable(displayParams.files[i].c_str(), displayParams.selectedLine == i)) {
                displayParams.selectedLine = i;
                displayParams.showEnvCubeDialog = false; // Close after selection
                displayParams.loadNewEnvCube = true;
                displayParams.newEnvCubeFileName = (textureFolder / displayParams.files[i]).string();
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Cancel")) {
            displayParams.showEnvCubeDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_None))
    {
        auto p = camera->getPosition();
        auto l = camera->getLookAt();
        ImGui::Separator();
        ImGui::Text("Camera Position: (%.1f,%.1f,%.1f)", p.x, p.y, p.z);
        ImGui::Text("Camera Direction: (%.2f,%.2f,%.2f)", l.x, l.y, l.z);
    }
    ImGui::Separator();
    const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
    const ImVec2 listSize(0.0f, rowHeight * 10.0f);

    if (ImGui::Button("Mesh Files"))
    {
        displayParams.showFileDialog = true;
        ImGui::OpenPopup("FileDialog");
    }
    if (ImGui::BeginPopupModal("FileDialog", &displayParams.showFileDialog)) {
        vector<MeshFile> meshFiles = engine->mstore.getMeshFiles();
        ImGui::Text("Loaded Mesh Files: %d", static_cast<int>(meshFiles.size()));

        ImGui::BeginChild("LoadedMeshFilesList", listSize, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const MeshFile& meshFile : meshFiles) {
            string filename = filesystem::path(meshFile.filename).filename().string();
            string label = filename + " [" + to_string(meshFile.meshes.size()) + "]";
            ImGui::TextUnformatted(label.c_str());
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button(displayParams.showAddMeshFileDialog ? "Hide Add Mesh File" : "Add Mesh File")) {
            displayParams.showAddMeshFileDialog = !displayParams.showAddMeshFileDialog;
        }

        if (displayParams.showAddMeshFileDialog) {
            engine->files.findAssetFolder("data");
            filesystem::path meshFolder = engine->files.getAssetFolderPath() / engine->files.MESH_PATH;
            displayParams.filePattern = "";
            displayParams.files = Util::getFilesMatchingPattern(meshFolder, displayParams.filePattern);

            ImGui::Text("Mesh folder: %s", meshFolder.string().c_str());
            ImGui::Text("Choose a mesh file:");
            const ImVec2 pickerSize(0.0f, rowHeight * 10.0f);

            static int selectedMeshLine = -1;
            ImGui::BeginChild("AvailableMeshFilesList", pickerSize, true, ImGuiWindowFlags_HorizontalScrollbar);
            for (int i = 0; i < static_cast<int>(displayParams.files.size()); ++i) {
                if (ImGui::Selectable(displayParams.files[i].c_str(), selectedMeshLine == i)) {
                    selectedMeshLine = i;
                    displayParams.loadNewMeshFile = true;
                    displayParams.newMeshFileName = (meshFolder / displayParams.files[i]).string();
                    displayParams.showAddMeshFileDialog = false;
                    ImGui::CloseCurrentPopup(); // close FileDialog after selection
                }
            }
            ImGui::EndChild();
        }

        if (ImGui::Button("Cancel")) {
            displayParams.showAddMeshFileDialog = false;
            displayParams.showFileDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_None))
    {
        fillStationaryModels();
        ImGui::Text("Choose an obejcts:");
        const ImVec2 pickerSize(0.0f, rowHeight * 10.0f);

        static int selectedObjLine = -1;
        ImGui::BeginChild("AvailableObjectList", pickerSize, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < static_cast<int>(displayParams.stationaryModels.size()); ++i) {
            auto& model = displayParams.stationaryModels[i];
            auto* so = engine->mstore.getSceneObject(i);
            int idx = i;
            string pos = std::to_string(so->pos.x) + ", " + std::to_string(so->pos.y) + ", " + std::to_string(so->pos.z);
            MeshFile meshFile;
            MeshFileEntry meshFileEntry;

            engine->mstore.getFileInfosForMesh(model.meshNumber, meshFile, meshFileEntry);
            string line = to_string(i) + " " + meshFileEntry.name + " [" + std::to_string(meshFileEntry.meshIndex) + "] " + pos;
            if (ImGui::Selectable(line.c_str(), selectedObjLine == i)) {
                selectedObjLine = i;
            }
        }
        ImGui::EndChild();
        // Object detail section
        if (selectedObjLine >= 0 && selectedObjLine < static_cast<int>(displayParams.stationaryModels.size())) {
            SceneObject* so = engine->mstore.getSceneObject(selectedObjLine);
            ImGui::Separator();
            ImGui::Text("Object %d details:", selectedObjLine);

            float pos[3] = { so->pos.x, so->pos.y, so->pos.z };
            float rot[3] = { so->rot.x, so->rot.y, so->rot.z };
            float scl    = so->scale.x;

            ImGui::DragFloat3("Position", pos,  0.01f);
            so->pos = { pos[0], pos[1], pos[2] };
            ImGui::DragFloat3("Rotation", rot,  0.01f);
            so->rot = { rot[0], rot[1], rot[2] };
            ImGui::DragFloat ("Scale",    &scl, 0.01f, 0.001f, 1000.0f);
            so->scale = glm::vec3(scl);
        }

        ImGui::Separator();
        if (ImGui::Button("Re-upload Stationary Objects")) {
            displayParams.reuploadStationaryObjects = true;
        }
    }
    ImGui::Separator();
    if (ImGui::Button("Mach was!")) {
        displayParams.addFixedObjectToScene = true;
    }
}

void SceneEditor::loadNewEnvCube(string textureFilePathName)
{
    filesystem::path filepath = textureFilePathName;
    string filename = filepath.filename().string();
    Log("WARNING: Loading new environment cube: " << filename << std::endl);

    engine->textureStore.freeTextureId("skyboxTexture");
    engine->textureStore.loadTexture(filename, "skyboxTexture");
    // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
    engine->textureStore.generateCubemaps("skyboxTexture");
    //engine->textureStore.freeTextureId(engine->textureStore.IRRADIANCE_TEXTURE_ID);
    //engine->textureStore.freeTextureId(engine->textureStore.PREFILTEREDENV_TEXTURE_ID);

    //engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
    //engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);

    engine->shaders.pbrShader.fillStandardFrameParams(frameParam);
    frameParam.scaleIBLAmbient = 1.0f; // adjust ambient light intensity
    engine->shaders.pbrShader.setFrameParam(frameParam, 0);
    engine->globalRendering.gpuMemory.flushAllBuffers();


    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);
}

void SceneEditor::addObjectToScene(const ObjectParams& params)
{
    MeshFlagsCollection flags;
    flags.setFlag(MeshFlags::MESHLET_GENERATE);
    //flags.setFlag(MeshFlags::RENDER_TYPE_MOVING);
    MStore& mstore = engine->mstore;
    MeshFile* loaded = engine->mstore.loadMesh("MirrorCube.glb", flags);
    //MeshFile* loaded = engine->mstore.loadMesh("Delfini6.glb", flags);
    auto meshInfo = mstore.getGPUMeshInfo(loaded->meshes[0].meshIndex);
    const auto meshMetadata = mstore.getMeshMetadata(loaded->meshes[0].meshIndex);
    SceneObject* object = engine->mstore.addObject(meshInfo->index, vec3(3.0f, 0.0f, 0.0f), flags);
    object->scale = vec3(1.0f);
    mat4 baseTransform = mat4(1.0); // get from gltf later
    GPUModel* gpuModel = engine->mstore.getGPUModel(object->index);
    object->prepareGPUModel(gpuModel, baseTransform);
    engine->shaders.pbrShader.recreateGlobalCommandBuffers();
    engine->shaders.pbrShader.initialUpload(true);
}

void SceneEditor::redoAllStationaryObjects()
{
    int num = engine->mstore.getUsedStationaryModelCount();
    for (int i = 0; i < num; ++i) {
        SceneObject* so = engine->mstore.getSceneObject(i);
        GPUModel* gpuModel = engine->mstore.getGPUModel(so->index);
        mat4 baseTransform = mat4(1.0); // get from gltf later
        so->prepareGPUModel(gpuModel, baseTransform);
        engine->globalRendering.gpuMemory.updateElement(BufferType::Models, *gpuModel, so->index);
        Log("Re-uploaded stationary object " << i << " at position: " << so->pos.x << ", " << so->pos.y << ", " << so->pos.z << std::endl);
    }
    engine->shaders.pbrShader.recreateGlobalCommandBuffers();
    engine->shaders.pbrShader.initialUpload(true);
}

void SceneEditor::loadNewMeshFile(string meshFilePathName)
{
    filesystem::path filepath = meshFilePathName;
    string filename = filepath.filename().string();
    Log("Loading new mesh file: " << filename << std::endl);

    MeshFlagsCollection flags;
    flags.setFlag(MeshFlags::MESHLET_GENERATE);
    engine->mstore.loadMesh(filename, flags);
}

void SceneEditor::fillStationaryModels()
{
    displayParams.stationaryModels.clear();
    int statModelNum = engine->mstore.getUsedStationaryModelCount();
    for (int i = 0; i < statModelNum; ++i) {
        GPUModel* object = engine->mstore.getGPUModel(i);
        displayParams.stationaryModels.push_back(*object);
    }
}