#include "mainheader.h"
#include "AppSupport.h"
#include "SceneEditor.h"
#include <random>

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

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    ls.color = vec3(1.0f);
    ls.position = vec3(75.0f, 90.5f, -20.0f);

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
    //if (displayParams.addFixedObjectToScene) {
    //    displayParams.addFixedObjectToScene = false;
    //    Log("Adding fixed object to scene" << endl);
    //    ObjectParams params;
    //    params.name = "FixedObject";
    //    params.meshFile = "path/to/mesh.obj";
    //    params.position = glm::vec3(0.0f, 0.0f, 0.0f);
    //    addObjectToScene(params);
    //}
    if (displayParams.showSunBeams && !displayParams.sunBeamsInitialized) {
        displayParams.sunBeamsInitialized = true;
        initSunRays();
    }

    if (displayParams.showSunBeams) {
        advanceSunRays(deltaSeconds, sunRayBox);
    }

    if (displayParams.addStationaryObjectToScene) {
        displayParams.addStationaryObjectToScene = false;
        addObjectToScene(displayParams.selectedLoadedMeshFileLine, false);
    }

    if (displayParams.addMovingObjectToScene) {
        displayParams.addMovingObjectToScene = false;
        addObjectToScene(displayParams.selectedLoadedMeshFileLine, true);
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

    // lines
    engine->shaders.lineShader.clearLocalLines(tr);
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
    // dynamic lines:
    if (displayParams.showSunBeams && !sunRays.empty()) {
        engine->shaders.lineShader.addOneTime(sunRays, tr);
    }
    engine->shaders.lineShader.prepareAddLines(tr);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

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

bool SceneEditor::showFileDialog(bool& loadedNewMeshFile, bool& selectedObjectForAdding, bool showAddObjectButton)
{
    loadedNewMeshFile = false;
    selectedObjectForAdding = false;
    const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
    const ImVec2 listSize(0.0f, rowHeight * 10.0f);

    if (ImGui::BeginPopupModal("FileDialog", &displayParams.showFileDialog)) {
        displayParams.fileDialogWasOpen = true;
        vector<MeshFile> meshFiles = engine->mstore.getMeshFiles();
        ImGui::Text("Loaded Mesh Files: %d", static_cast<int>(meshFiles.size()));

        ImGui::BeginChild("LoadedMeshFilesList", listSize, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < static_cast<int>(meshFiles.size()); ++i) {
            const MeshFile& meshFile = meshFiles[i];
            string filename = filesystem::path(meshFile.filename).filename().string();
            string label = filename + " [" + to_string(meshFile.meshes.size()) + "]";

            if (ImGui::Selectable(label.c_str(), displayParams.selectedLoadedMeshFileLine == i)) {
                displayParams.selectedLoadedMeshFileLine = i;
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        const bool hasMeshSelection =
            displayParams.selectedLoadedMeshFileLine >= 0 &&
            displayParams.selectedLoadedMeshFileLine < static_cast<int>(meshFiles.size());

        if (!hasMeshSelection) {
            ImGui::BeginDisabled();
        }
        if (showAddObjectButton) {
            if (ImGui::Button("Add Object")) {
                displayParams.fileDialogSelectedObjectForAddingDuringSession = true;
                displayParams.showFileDialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        //ImGui::SameLine();
        //if (ImGui::Button("Add Moving Object")) {
        //    displayParams.addMovingObjectToScene = true;
        //    displayParams.fileDialogSelectedObjectForAddingDuringSession = true;
        //}
        if (!hasMeshSelection) {
            ImGui::EndDisabled();
        }

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
                    displayParams.fileDialogLoadedNewMeshFileDuringSession = true;
                    displayParams.showFileDialog = false;
                    ImGui::CloseCurrentPopup();
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

    const bool dialogClosed = displayParams.fileDialogWasOpen && !displayParams.showFileDialog;
    if (dialogClosed) {
        loadedNewMeshFile = displayParams.fileDialogLoadedNewMeshFileDuringSession;
        selectedObjectForAdding = displayParams.fileDialogSelectedObjectForAddingDuringSession;
        displayParams.fileDialogLoadedNewMeshFileDuringSession = false;
        displayParams.fileDialogSelectedObjectForAddingDuringSession = false;
        displayParams.fileDialogWasOpen = false;
    }

    return dialogClosed;
}

void SceneEditor::buildCustomUI() {
    if (ImGui::CollapsingHeader("Display Tweaks", ImGuiTreeNodeFlags_None))
    {
        ImGui::Checkbox("Sun Beams", &displayParams.showSunBeams);
    }
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

    static bool showAddObjectButtonInFileDialog = false;
    static bool addObjectToMovingSection = false;

    if (ImGui::Button("Mesh Files"))
    {
        displayParams.showFileDialog = true;
        displayParams.fileDialogLoadedNewMeshFileDuringSession = false;
        displayParams.fileDialogSelectedObjectForAddingDuringSession = false;
        displayParams.fileDialogWasOpen = false;
        showAddObjectButtonInFileDialog = false;
        ImGui::OpenPopup("FileDialog");
    }

    bool loadedNewMeshFile = false;
    bool selectedObjectForAdding = false;
    const bool fileDialogClosed = showFileDialog(loadedNewMeshFile, selectedObjectForAdding, showAddObjectButtonInFileDialog);
    if (fileDialogClosed && selectedObjectForAdding) {
        if (addObjectToMovingSection) {
            displayParams.addMovingObjectToScene = true;
        }
        else {
            displayParams.addStationaryObjectToScene = true;
        }
    }
    if (ImGui::CollapsingHeader("Stationary Objects", ImGuiTreeNodeFlags_None))
    {
        fillStationaryModels();
        if (ImGui::Button("Add Object to Stationary Objects")) {
            displayParams.showFileDialog = true;
            displayParams.fileDialogLoadedNewMeshFileDuringSession = false;
            displayParams.fileDialogSelectedObjectForAddingDuringSession = false;
            displayParams.fileDialogWasOpen = false;
            showAddObjectButtonInFileDialog = true;
            addObjectToMovingSection = false;
            ImGui::OpenPopup("FileDialog");
        }
        ImGui::Text("Choose an object:");
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
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("Changing parameters for stationary objects requires re-uploading all stationary objects.");
        }
    }
    // Add this new section right after the existing "Stationary Objects" block:

    if (ImGui::CollapsingHeader("Moving Objects", ImGuiTreeNodeFlags_None))
    {
        fillMovingModels();
        if (ImGui::Button("Add Object to Moving Objects")) {
            displayParams.showFileDialog = true;
            displayParams.fileDialogLoadedNewMeshFileDuringSession = false;
            displayParams.fileDialogSelectedObjectForAddingDuringSession = false;
            displayParams.fileDialogWasOpen = false;
            showAddObjectButtonInFileDialog = true;
            addObjectToMovingSection = true;
            ImGui::OpenPopup("FileDialog");
        }
        ImGui::Text("Choose an object:");
        const ImVec2 pickerSize(0.0f, rowHeight * 10.0f);

        static int selectedMovingObjLine = -1;
        ImGui::BeginChild("AvailableMovingObjectList", pickerSize, true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < static_cast<int>(displayParams.movingModels.size()); ++i) {
            auto& model = displayParams.movingModels[i];
            auto* so = engine->mstore.getMovingSceneObject(i);
            string pos = std::to_string(so->pos.x) + ", " + std::to_string(so->pos.y) + ", " + std::to_string(so->pos.z);

            MeshFile meshFile;
            MeshFileEntry meshFileEntry;
            engine->mstore.getFileInfosForMesh(model.meshNumber, meshFile, meshFileEntry);

            string line = to_string(i) + " " + meshFileEntry.name + " [" + std::to_string(meshFileEntry.meshIndex) + "] " + pos;
            if (ImGui::Selectable(line.c_str(), selectedMovingObjLine == i)) {
                selectedMovingObjLine = i;
            }
        }
        ImGui::EndChild();

        // Object detail section (live update)
        if (selectedMovingObjLine >= 0 && selectedMovingObjLine < static_cast<int>(displayParams.movingModels.size())) {
            SceneObject* so = engine->mstore.getMovingSceneObject(selectedMovingObjLine);
            GPUModelParam* gpuModelParam = engine->mstore.getGPUModelParam(so->index);
            ImGui::Separator();
            ImGui::Text("Moving Object %d details:", selectedMovingObjLine);

            float pos[3] = { so->pos.x, so->pos.y, so->pos.z };
            float rot[3] = { so->rot.x, so->rot.y, so->rot.z };
            float scl = so->scale.x;

            ImGui::DragFloat3("Position##moving", pos, 0.01f);
            so->pos = { pos[0], pos[1], pos[2] };

            ImGui::DragFloat3("Rotation##moving", rot, 0.01f);
            so->rot = { rot[0], rot[1], rot[2] };

            ImGui::DragFloat("Scale##moving", &scl, 0.01f, 0.001f, 1000.0f);
            so->scale = glm::vec3(scl);

            // live GPU update (no re-upload button)
            GPUModel* gpuModel = engine->mstore.getGPUMovingModel(so->index);
            mat4 baseTransform = mat4(1.0f);
            so->prepareGPUModel(gpuModel, baseTransform);
            engine->globalRendering.gpuMemory.updateElement(BufferType::ModelsMoving, *gpuModel, so->index);

            // detect change
            if (gpuModelParam->pos != so->pos || gpuModelParam->rot != so->rot || gpuModelParam->scale != so->scale) {
                gpuModelParam->pos = so->pos;
                gpuModelParam->rot = so->rot;
                gpuModelParam->scale = so->scale;
                //Log("change!!\n");
                engine->globalRendering.gpuMemory.updateElement(BufferType::ModelsParam, *gpuModelParam, so->index);
                engine->globalRendering.gpuMemory.flushBuffer(BufferType::ModelsParam);
            }
            //Log("param update: pos(" << gpuModelParam->pos.x << ", " << gpuModelParam->pos.y << ", " << gpuModelParam->pos.z << "), rot(" << gpuModelParam->rot.x << ", " << gpuModelParam->rot.y << ", " << gpuModelParam->rot.z << "), scale(" << gpuModelParam->scale.z<< ")" << std::endl);

        }
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

void SceneEditor::addObjectToScene(int meshFileIndex, bool moving)
{
    vector<MeshFile> meshFiles = engine->mstore.getMeshFiles();
    if (meshFileIndex < 0 || meshFileIndex >= static_cast<int>(meshFiles.size())) {
        Error("SceneEditor::addObjectToScene: invalid mesh file selection");
        return;
    }

    const MeshFile& selectedFile = meshFiles[meshFileIndex];
    if (selectedFile.meshes.empty()) {
        Error("SceneEditor::addObjectToScene: selected mesh file has no meshes");
        return;
    }

    MeshFlagsCollection flags;
    flags.setFlag(MeshFlags::MESHLET_GENERATE);
    if (moving) {
        flags.setFlag(MeshFlags::RENDER_TYPE_MOVING);
    }

    const int meshIndex = selectedFile.meshes[0].meshIndex;
    GPUMeshInfo* meshInfo = engine->mstore.getGPUMeshInfo(meshIndex);
    if (meshInfo == nullptr) {
        Error("SceneEditor::addObjectToScene: failed to resolve mesh info");
        return;
    }

    SceneObject* object = engine->mstore.addObject(meshInfo->index, vec3(0.0f, 0.0f, 0.0f), flags);
    if (object == nullptr) {
        Error("SceneEditor::addObjectToScene: failed to add object");
        return;
    }

    object->flags = flags;
    object->scale = vec3(1.0f);

    mat4 baseTransform = mat4(1.0f);
    GPUModel* gpuModel = moving
        ? engine->mstore.getGPUMovingModel(object->index)
        : engine->mstore.getGPUModel(object->index);

    object->prepareGPUModel(gpuModel, baseTransform);

    if (moving) {
        engine->globalRendering.gpuMemory.updateElement(BufferType::ModelsMoving, *gpuModel, object->index);
    }
    else {
        engine->globalRendering.gpuMemory.updateElement(BufferType::Models, *gpuModel, object->index);
    }

    if (moving) {
        redoAllMovingObjects();
    } else {
        redoAllStationaryObjects();
    }
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
        //Log("Re-uploaded stationary object " << i << " at position: " << so->pos.x << ", " << so->pos.y << ", " << so->pos.z << std::endl);
    }
    engine->shaders.pbrShader.recreateGlobalCommandBuffers();
    engine->shaders.pbrShader.initialUpload(true);
}

void SceneEditor::redoAllMovingObjects()
{
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
    engine->shaders.pbrShader.recreateGlobalCommandBuffers();
    engine->shaders.pbrShader.initialUpload(true);
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

void SceneEditor::fillMovingModels()
{
    displayParams.movingModels.clear();
    int movingModelNum = static_cast<int>(engine->globalRendering.gpuMemory.getElementCount(BufferType::ModelsMoving));
    for (int i = 0; i < movingModelNum; ++i) {
        GPUModel* object = engine->mstore.getGPUMovingModel(i);
        displayParams.movingModels.push_back(*object);
    }
}

void SceneEditor::initSunRays()
{
    sunRays.clear();
    // Sun-ray simulation box (same dimensions as world extents)
    sunRayBox.min = glm::vec3(-1024.0f, -191.0f, -1024.0f);
    sunRayBox.max = glm::vec3(1024.0f, 191.0f, 1024.0f);

    // Initial sun rays
    vec4 sunColor = vec4(ls.color, 1.0f); // warm sunlight color
    // calculate sun direction based on light source position:
    vec3 sunDirection = glm::normalize(ls.position * -1.0f);
    setupSunRays(sunDirection, 300, 35.0f, 22.0f, sunColor);
}

bool SceneEditor::isInsideBox(const glm::vec3& p, const BoundingBox& box)
{
    return p.x >= box.min.x && p.x <= box.max.x &&
        p.y >= box.min.y && p.y <= box.max.y &&
        p.z >= box.min.z && p.z <= box.max.z;
}

LineDef SceneEditor::createRandomSunRay(const BoundingBox& box) const
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dx(box.min.x, box.max.x);
    std::uniform_real_distribution<float> dy(box.min.y, box.max.y);
    std::uniform_real_distribution<float> dz(box.min.z, box.max.z);

    glm::vec3 start(dx(rng), dy(rng), dz(rng));
    glm::vec3 end = start + sunRayDirection * sunRayLength;
    return LineDef{ start, end, sunRayColor };
}

void SceneEditor::setupSunRays(const glm::vec3& sunDirection, int numberRays, float rayLength, float raySpeed, const glm::vec4& rayColor)
{
    sunRayDirection = glm::length(sunDirection) > 0.0001f ? glm::normalize(sunDirection) : glm::vec3(0.0f, -1.0f, 0.0f);
    sunRayLength = glm::max(0.01f, rayLength);
    sunRaySpeed = glm::max(0.0f, raySpeed);
    sunRayColor = rayColor;

    sunRays.clear();
    sunRays.reserve(glm::max(0, numberRays));
    for (int i = 0; i < numberRays; ++i) {
        sunRays.push_back(createRandomSunRay(sunRayBox));
    }
}

void SceneEditor::advanceSunRays(float deltaSeconds, const BoundingBox& simulationBox)
{
    if (sunRays.empty() || deltaSeconds <= 0.0f) {
        return;
    }

    const glm::vec3 step = sunRayDirection * sunRaySpeed * deltaSeconds;

    for (auto& ray : sunRays) {
        ray.start += step;
        ray.end = ray.start + sunRayDirection * sunRayLength;
        ray.color = sunRayColor;

        // Respawn ray when it leaves the simulation box
        if (!isInsideBox(ray.start, simulationBox)) {
            ray = createRandomSunRay(simulationBox);
        }
    }
}