#include "mainheader.h"
#include "AppSupport.h"
#include "MeshManager.h"

using namespace std;
using namespace glm;

void MeshManager::run(ContinuationInfo* cont)
{
    Log("MeshManager started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        engine->appname = "Mesh Manager";
        // camera initialization
        //initCamera(glm::vec3([-0.0386716 0.57298 1.71695]), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //initCamera(glm::vec3(-0.0386716f, 0.5f, 1.71695f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //initCamera(glm::vec3(-0.0386716f, 0.2f, 0.51695f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //initCamera(glm::vec3(-2.10783f, 0.56567f, -0.129275f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        initCamera(glm::vec3(-0.204694f, 0.198027f, 20.520922f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);
        //getFirstPersonCameraPositioner()->setMaxSpeed(10.1f);
        getHMDCameraPositioner()->setMaxSpeed(0.1f);

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.01f, 5000.0f);
        Movement mv;
        camera->setConstantSpeed(mv.walkSpeedMS);


        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)
            .addShader(shaders.pbrShader)
            .addShader(shaders.lineShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        //shaders.pbrShader.setWireframe();
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("MeshManager ended" << endl);
}

void MeshManager::init() {
    engine->sound.init(false);

    engine->textureStore.generateBRDFLUT();


    engine->objectStore.createGroup("group");

    //object->enableDebugGraphics = true;
    //if (alterObjectCoords) {
    //    // turn upside down
    //    object->rot() = vec3(PI_half, 0.0, 0.0f);
    //}

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    world.createWorldGridAndCopyToLineVector(grid1, 1.0f);
    world.createWorldGridAndCopyToLineVector(grid10, 10.0f);
    world.createWorldGridAndCopyToLineVector(grid100, 100.0f);

    //engine->textureStore.loadTexture("nebula.ktx2", "skyboxTexture");
    engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
    //engine->textureStore.generateCubemaps("skyboxTexture");
    engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
    engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);

    PBRShader::LightSource ls;
    ls.color = vec3(1.0f);
    //ls.position = vec3(75.0f, 0.5f, -20.0f);
    //ls.position = vec3(75.0f, -10.5f, -40.0f);
    ls.position = vec3(75.0f, 30.5f, -40.0f);
    engine->shaders.pbrShader.changeLightSource(ls.color, ls.position);
    float curLod[10] = { 0, 1, 5, 10, 15, 25, 30, 50, 70, 150 };
    memcpy(lod, curLod, sizeof(float) * 10);

    engine->shaders.pbrShader.initialUpload();
    // window creation
    prepareWindowOutput("Mesh Manager");
    engine->presentation.startUI();

    // load and play music
    engine->sound.openSoundFile(Sound::SHADED_PATH_JINGLE_FILE, Sound::SHADED_PATH_JINGLE);
    engine->sound.playSound(Sound::SHADED_PATH_JINGLE, SoundCategory::MUSIC);

}

void MeshManager::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void MeshManager::prepareFrame(FrameResources* fr)
{
    FrameResources& tr = *fr;
    double seconds = engine->gameTime.getTimeSeconds();
    if ((old_seconds > 0.0f && old_seconds == seconds) || old_seconds > seconds) {
        Error("APP TIME ERROR - should not happen");
        return;
    }
    double deltaSeconds = seconds - old_seconds;

    if (uiCameraSpeed != oldUiCameraSpeed) {
        Movement mv;
        oldUiCameraSpeed = uiCameraSpeed;
        if (uiCameraSpeed == 1) {
            camera->setConstantSpeed(mv.runSpeedMS);
        } else if (uiCameraSpeed == 2) {
            camera->setConstantSpeed(mv.fallSpeedMS);
        } else {
            camera->setConstantSpeed(mv.walkSpeedMS);
        }
    }
    updateCameraPositioners(deltaSeconds);
    old_seconds = seconds;

    // file handling:
    if (loadNewFile) {
        // load new file
        loadNewFile = false;
        // convert filename to path and check if file exists
        std::filesystem::path filepath = engine->files.findFile(newFileName, FileCategory::MESH, false);
        if (filepath.empty() || !std::filesystem::exists(filepath)) {
            Log("File does not exist: " << newFileName << endl);
            return;
        }
        Log("Loading new file: " << filepath.filename() << endl);
        displayNoMeshletDataWarning = false;
        //engine->meshStore.loadMesh(filepath.filename().string(), "newid", MeshFlagsCollection(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER));
        // generate new id for each loaded object:
        string newid = "newid";// +to_string(loadObjectNum++);
        for (int i = 0; i < loadObjectNum; i++) {
            newid += "X";
        }
        engine->meshStore.loadMesh(filepath.filename().string(), newid);
        //engine->meshStore.loadMesh(filepath.filename().string(), newid, MeshFlagsCollection(MeshFlags::MESH_TYPE_NO_TEXTURES));
        if (object != nullptr) {
            object->enabled = false;
        }
        auto* coll = engine->meshStore.getMeshCollection(newid);
        objects.clear();
        simObjects.clear();
        Log("Loaded " << coll->meshInfos.size() << " meshes from file" << endl);
        float xpos = 0.0f;
        for (auto* mi : coll->meshInfos) {
            Log("    Mesh: " << mi->id << " triangles: " << mi->indices.size()/3 << " vertices: " << mi->vertices.size() << endl);
            vec3 pos = vec3(xpos, 0.0f, 0.0f);
            BoundingBox box;
            mi->getBoundingBox(box);
            float width = box.max.x - box.min.x;
            //xpos += width;
            auto* wo = object = engine->objectStore.addObject("group", mi->id, pos);
            objects.push_back(wo);
            pos.x = -2.0f;
            wo = engine->objectStore.addObject("group", mi->id, pos);
            wo->enabled = false;
            simObjects.push_back(wo);
        }
        // add test object for GPU LOD:
        enableGpuLodObject = false;
        if (objects.size() > 0 && engine->meshStore.isGPULodCompatible(objects[0])) {
            gpuLodObject = engine->objectStore.addObject("group", coll->meshInfos[0]->id, vec3(-2.0f, 2.0f, 0.0f));
            gpuLodObject->enabled = false;
            gpuLodObject->useGpuLod = true;
        }

        if (!object->mesh->meshletStorageFileFound) {
            Log("ERROR: Meshlet storage file not found for this object, meshlets will not be used for rendering" << endl);
            displayNoMeshletDataWarning = true;
        }
        meshCollection = coll;
        //object->enableDebugGraphics = true;
        engine->shaders.pbrShader.initialUpload();
        engine->shaders.pbrShader.recreateGlobalCommandBuffers();
    }
    // new mesh selected from collection:
    if (meshSelectedFromCollection != nullptr) {
        // disable old object
        if (object != nullptr) {
            //object->enabled = false;
            object = nullptr;
        }
        // find position for new object in objects vector:
        for (size_t i = 0; i < objects.size(); i++) {
            if (objects[i]->mesh == meshSelectedFromCollection) {
                object = objects[i];
                break;
            }
        }
        assert(object != nullptr);
        //object = engine->objectStore.addObject("group", meshSelectedFromCollection->id, vec3(0.0f));
        object->enabled = true;
        meshSelectedFromCollection = nullptr;
        modelRotation = object->rot() / vec3(PI_half);
        modelTranslation = object->pos();
    }
    if (regenerateMeshletData) {
        regenerateMeshletData = false;
        Log("Regenerating meshlet data for file: " << newFileName << endl);
        string newid = "newid" + to_string(loadObjectNum++);
        std::filesystem::path filepath = newFileName;
        MeshFlagsCollection meshFlags = MeshFlagsCollection(MeshFlags::MESHLET_GENERATE);
        engine->meshStore.loadMesh(filepath.filename().string(), newid, meshFlags);
        bool ok = engine->meshStore.writeMeshletStorageFile(newid, filepath.filename().string());
        regenerationFinished = true;
    }

    if (spinningBox == false && seconds > 4.0f) {
        spinningBox = true; // start spinning the logo after 4s
        spinTimeSeconds = seconds;
    }
    if (applySetupObjects) {
        applySetupObjects = false;
        if (objects.size() != 10) {
            Error("Loaded objects not suitable for LOD claculations");
        }
        // scale to have 1m cube diameter for LOD 0 object:
        BoundingBox box;
        objects[0]->getBoundingBoxWorld(box, objects[0]->mesh->baseTransform);
        float diameter = length(box.max - box.min);
        float scale = 1.732f / diameter;
        modelScale = scale;
        // we base all calculations on LOD 0:
        for (int i = 0; i < 10; i++) {
            objects[i]->scale() = vec3(scale);
            //objects[i]->rot() = vec3(0.0f, PI_half, 0.0f);
            objects[i]->pos() = vec3(i * 1.0f, 0.0f, 0.0f);
        }
        object = nullptr;
    }
    if (applyLOD) {
        applyLOD = false;
        if (objects.size() != 10) {
            Error("Loaded objects not suitable for LOD claculations");
        }
        // position the objects according to LOD distances, starting from z = 0 line:
        for (int i = 0; i < 10; i++) {
            objects[i]->pos().z = -lod[i];
        }
        // 1 5 10 15 25 30 50 70 150
    }
    if (simObjects.size() == 10) {
        if (simulateLOD) {
            vec3 camPos = camera->getPosition();
            vec3 objPos = simObjects[0]->pos();
            //objPos.y = 0.0f; ???
            float dist = length(camPos - objPos);
            int lodLevel = Util::calculateLODIndex(lod, dist);
            //make object visible:
            for (int i = 0; i < 10; i++) {
                simObjects[i]->enabled = false;
            }
            simObjects[lodLevel]->enabled = true;
            // update UI:
            simLODLevel = lodLevel;
            lodDistance = dist;
        }
        else {
            for (int i = 0; i < 10; i++) {
                simObjects[i]->enabled = false;
            }
        }
    }
    if (gpuLodObject != nullptr) {
        if (enableGpuLodObject) {
            gpuLodObject->enabled = true;
        }
        else {
            gpuLodObject->enabled = false;
        }
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
    // change individual objects position:
    //auto grp = engine->objectStore.getGroup("knife_group");
    engine->shaders.lineShader.clearLocalLines(tr);
    vector<LineDef> boundingBoxes;
    for (auto& wo : engine->objectStore.getSortedList()) {
        PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        //buf->flags |= PBRShader::MODEL_RENDER_FLAG_USE_VERTEX_COLORS; // enable simple shading using vertex colors
        if (!wo->enabled) {
            buf->disableRendering();
            buf->objPos = wo->pos(); // get rid of uninitialized object position warning
            continue;
        } else {
            buf->enableRendering();
            buf->objPos = wo->pos();
        }
        buf->params[0].intensity = sunIntensity; // adjust sun light intensity
        //buf->params[0].scaleIBLAmbient = 2.0f; // adjust IBL intensity
        if (addSunDirBeam) {
            // add a line to indicate sun direction:
            vec3 sunDir = normalize(buf->params[0].lightDir);
            vec3 start = wo->pos();
            vec3 end = start + sunDir * 500.0f;
            LineDef beam(start, end, Colors::Red);
            vector<LineDef> beams;
            beams.push_back(beam);
            PBRShader::LightSource* ls = engine->shaders.pbrShader.getLightSource();
            beam.start = vec3(0.0f);
            beam.end = ls->position * 100.0f;
            beam.color = Colors::Green;
            beams.push_back(beam);
            engine->shaders.lineShader.addOneTime(beams, tr);
        }
        if (wo == object && object != nullptr) {
            // do only manipulations for selected object
            wo->pos() = modelTranslation;
            // adapt with UI rotation input:
            wo->rot() = modelRotation * vec3(PI_half);
            wo->scale() = vec3(modelScale);
        }
        if (changeAllObjects) {
            wo->rot() = modelRotation * vec3(PI_half);
            wo->scale() = vec3(modelScale);
        }
        mat4 modeltransform;
        modeltransform = wo->mesh->baseTransform;
        if (spinningBox) {
            // Define a constant rotation speed (radians per second)
            double rotationSpeed = glm::radians(4.0f);

            // Calculate the rotation angle based on the elapsed time
            //float rotationAngle = rotationSpeed * (seconds - spinTimeSeconds);
            float rotationAngle = rotationSpeed * deltaSeconds;

            // Apply the rotation to the modeltransform matrix
            if (doRotation) {
                wo->rot().y += rotationAngle;
                // push actual rotation back to UI:
                modelRotation = wo->rot() / vec3(PI_half);
            }
        }

        wo->calculateStandardModelTransform(modeltransform);
        buf->model = modeltransform;
        // uncomment to enable more debug meshlet displays:
        //engine->meshStore.debugRenderMeshletFromBuffers(wo, tr, modeltransform);
        // vertices display for meshes with no meshlet data available:
        if (!wo->mesh->meshletStorageFileFound) {
            wo->enableDebugGraphics = true;
            showMeshWireframe = true;
        }
        if (showBoundingBox || showMeshletBoundingBoxes || showMeshWireframe) {
            wo->enableDebugGraphics = true;
            engine->meshStore.debugGraphics(wo, tr, modeltransform, showBoundingBox, showMeshWireframe, false, showMeshletBoundingBoxes); // bb and wireframe
        }
    }
    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
    // dynamic lines:
    if (planeGrid) {
        if (gridSpacing == 1) {
            engine->shaders.lineShader.addOneTime(grid1, tr);
        }
        else if (gridSpacing == 10) {
            engine->shaders.lineShader.addOneTime(grid10, tr);
        }
        else if (gridSpacing == 100) {
            engine->shaders.lineShader.addOneTime(grid100, tr);
        }
    }
    engine->shaders.lineShader.prepareAddLines(tr);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    postUpdatePerFrame(tr);
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void MeshManager::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        //engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
        if (engine->sound.enabled) {
            engine->sound.Update(camera);
        }
        // draw lines
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
    }
    else if (topic == 1) {
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
        //Log("Loader::drawFrame: PBR shader command buffers added" << endl);
    }
}

void MeshManager::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void MeshManager::processImage(FrameResources* fr)
{
    present(fr);
}

bool MeshManager::shouldClose()
{
    return shouldStopEngine;
}

void MeshManager::handleInput(InputState& inputState)
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
            doRotation = !doRotation;
        }
    }
    AppSupport::handleInput(inputState);
}

namespace fs = std::filesystem;

std::vector<std::string> getFilesMatchingPattern(const fs::path& folder, const std::string& pattern) {
    std::vector<std::string> result;
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            // Simple pattern match: ends with extension
            if (pattern == "*" || entry.path().filename().string().find(pattern) != std::string::npos) {
                result.push_back(entry.path().filename().string());
            }
        }
    }
    return result;
}
void MeshManager::buildCustomUI() {
    static bool showLineSelector = false;
    static int selectedLine = -1;
    static vector<string> files;
    static string currentFilePattern = "_cmp.glb";
    static char patternBuffer[64] = "_cmp.glb";

    // --- Display max mesh count and max storage size in GB ---
    if (engine) {
        double maxStorageGB = static_cast<double>(engine->getMeshStorageSize()) / (1024.0 * 1024.0 * 1024.0);
        double usedStorageGB = static_cast<double>(engine->meshStore.getUsedStorageSize()) / (1024.0 * 1024.0 * 1024.0);
        uint64_t maxMeshes = engine->getMaxMeshes();
        size_t usedMeshes = engine->meshStore.getUsedMeshesCount();

        ImGui::SeparatorText("Storage");
        ImGui::Text("Max: %.2f GB", maxStorageGB); ImGui::SameLine();
        ImGui::Text("Used: %.2f GB", usedStorageGB);
        ImGui::SeparatorText("Meshes");
        ImGui::Text("Max: %llu", maxMeshes);  ImGui::SameLine();
        ImGui::Text("Used: %u", usedMeshes);
    }
    ImGui::Separator();
    //ImGui::Checkbox("Show Wireframe if meshlet file missing", &showMeshWireframe);
    // Input field for file pattern
    ImGui::InputText("File Pattern", patternBuffer, sizeof(patternBuffer));
    currentFilePattern = patternBuffer;
    if (ImGui::Button("Select GLB file from data folder")) {
        showLineSelector = true;
        ImGui::OpenPopup("File Selection");
    }
    if (ImGui::BeginPopupModal("File Selection", &showLineSelector)) {
        // load file list from data folder
        engine->files.findAssetFolder("data"); // maybe let the user change asset folder name?
        filesystem::path meshFolder = engine->files.getAssetFolderPath() / engine->files.MESH_PATH;
        files = getFilesMatchingPattern(meshFolder, currentFilePattern);
        ImGui::Text("Data folder: %s", meshFolder.string().c_str());
        ImGui::Text("Choose a file:");
        ImGui::Separator();

        for (int i = 0; i < files.size(); ++i) {
            if (ImGui::Selectable(files[i].c_str(), selectedLine == i)) {
                selectedLine = i;
                showLineSelector = false; // Close after selection
                loadNewFile = true;
                newFileName = (meshFolder / files[i]).string();
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Cancel")) {
            showLineSelector = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    string filename;
    // Optionally, display the selected line
    if (selectedLine >= 0) {
        filename = files[selectedLine];
        ImGui::Text("Selected: %s", filename.c_str());
        if (displayNoMeshletDataWarning) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "No meshlet data found. \nReverting to vertices display");
        }
    }
    static int clicked = 0;
    if (regenerationFinished) {
        regenerationFinished = false;
        clicked = 0;
    }
    if (!newFileName.empty()) {
        if (ImGui::Button("Generate Meshlet Data"))
            clicked++;
        ImGui::SetItemTooltip(newFileName.c_str());
        if (clicked == 1)
        {
            ImGui::SameLine();
            ImGui::Text("Generating... (see log for progress)");
            regenerateMeshletData = true;
            clicked = 2; // prevent multiple clicks
        }
    }
    bool selectEnabled = (meshCollection != nullptr && meshCollection->meshInfos.size() > 1);
    ImGui::BeginDisabled(!selectEnabled);
    if (ImGui::TreeNode("Select Mesh"))
    {
        static int selected = -1;
        static int prevSelected = -1;
        for (int n = 0; n < meshCollection->meshInfos.size(); n++)
        {
            char buf[128];
            string s = meshCollection->meshInfos[n]->name;
            sprintf(buf, "%s %d", s.c_str(), n);
            if (ImGui::Selectable(buf, selected == n))
                selected = n;
        }
        if (selected != prevSelected && selected >= 0) {
            meshSelectedFromCollection = meshCollection->meshInfos[selected];
            prevSelected = selected;
        }
        ImGui::TreePop();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Select a mesh from the collection contained in %s", filename.c_str());
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Zero-Plane Grid");
    ImGui::Checkbox("enable 2 km square grid", &planeGrid);

    ImGui::RadioButton("1 m", &gridSpacing, 1); ImGui::SameLine();
    ImGui::RadioButton("10 m", &gridSpacing, 10); ImGui::SameLine();
    ImGui::RadioButton("100 m", &gridSpacing, 100);

    ImGui::SeparatorText("Camera Speed");
    ImGui::RadioButton("walk (3.5 km/h)", &uiCameraSpeed, 0); ImGui::SameLine();
    ImGui::RadioButton("run (10 km/h)", &uiCameraSpeed, 1); ImGui::SameLine();
    ImGui::RadioButton("fall (200 km/h)", &uiCameraSpeed, 2);
    if (ImGui::CollapsingHeader("LOD", ImGuiTreeNodeFlags_None))
    {
        if (ImGui::Button("Setup Objects")) {
            applySetupObjects = true;
        }
        ImGui::SetItemTooltip("Rescale to 1m width and place all objects on Zero point and to the right");
        ImGui::SeparatorText("LOD Definition");
        ImGui::InputFloat("LOD 1", &lod[1], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 2", &lod[2], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 3", &lod[3], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 4", &lod[4], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 5", &lod[5], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 6", &lod[6], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 7", &lod[7], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 8", &lod[8], 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("LOD 9", &lod[9], 0.1f, 1.0f, "%.3f");
        if (ImGui::Button("Apply LOD")) {
            applyLOD = true;
        }
        ImGui::Checkbox("Simulate LOD", &simulateLOD); ImGui::SameLine();
        ImGui::Checkbox("Enable GPU LOD Object", &enableGpuLodObject);
        ImGui::BeginDisabled(!simulateLOD);
        ImGui::Text("Camera Distance: %.3f", lodDistance); ImGui::SameLine();
        ImGui::Text("LOD Level: %d", simLODLevel);
        ImGui::EndDisabled();
    }
    if (ImGui::CollapsingHeader("More Options", ImGuiTreeNodeFlags_None))
    {
        ImGui::Checkbox("Change All Objects", &changeAllObjects);
        ImGui::SetItemTooltip("Apply LOD switching to object left of Zero point. Camera distance and LOD level are displayed.");
        ImGui::InputFloat("Sun Intensity", &sunIntensity, 1.0f, 5.0f, "%.3f");
        ImGui::Checkbox("Debug Sun Position", &addSunDirBeam);
        ImGui::Checkbox("Mesh Wireframe", &showMeshWireframe);
        ImGui::SameLine();
        ImGui::Checkbox("Bounding Box", &showBoundingBox);
        ImGui::SameLine();
        ImGui::Checkbox("Meshlet Bounding Boxes", &showMeshletBoundingBoxes);
        ImGui::SeparatorText("Object Rotation (as increments to PI/2)");
        ImGui::InputFloat("X Axis", &modelRotation.x, 0.5f, 1.0f, "%.3f");
        ImGui::InputFloat("Y Axis", &modelRotation.y, 0.5f, 1.0f, "%.3f");
        ImGui::InputFloat("Z Axis", &modelRotation.z, 0.5f, 1.0f, "%.3f");
        ImGui::SeparatorText("Object Position");
        ImGui::InputFloat("X", &modelTranslation.x, 0.5f, 10.0f, "%.3f");
        ImGui::InputFloat("Y", &modelTranslation.y, 0.5f, 10.0f, "%.3f");
        ImGui::InputFloat("Z", &modelTranslation.z, 0.5f, 10.0f, "%.3f");
        ImGui::SeparatorText("Object Scale");
        ImGui::InputFloat("Uniform Scale", &modelScale, 0.1f, 1.0f, "%.3f");
        if (object != nullptr) {
            ImGui::Text("Current Object: %s", object->mesh->name.c_str());
            BoundingBox box;
            mat4 modeltransform;
            object->calculateStandardModelTransform(modeltransform);
            object->getBoundingBoxWorld(box, modeltransform);
            ImGui::Text("BBOX min: %.3f %.3f %.3f", box.min.x, box.min.y, box.min.z);
            ImGui::Text("     max: %.3f %.3f %.3f", box.max.x, box.max.y, box.max.z);
        }
    }
}
