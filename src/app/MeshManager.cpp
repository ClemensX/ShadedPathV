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
        initCamera(glm::vec3(-0.204694f, 0.198027f, 2.520922f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);
        //getFirstPersonCameraPositioner()->setMaxSpeed(10.1f);
        getHMDCameraPositioner()->setMaxSpeed(0.1f);

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

    MeshFlagsCollection meshFlags = MeshFlagsCollection(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER);
    //meshFlags.setFlag(MeshFlags::MESHLET_DEBUG_COLORS);
    engine->meshStore.loadMeshCylinder("LogoBox", meshFlags, engine->textureStore.BRDFLUT_TEXTURE_ID, true); alterObjectCoords = false;

    engine->objectStore.createGroup("group");
    object = engine->objectStore.addObject("group", "LogoBox", vec3(-0.2f, 0.2f, 0.2f));

    //object->enableDebugGraphics = true;
    if (alterObjectCoords) {
        // turn upside down
        object->rot() = vec3(PI_half, 0.0, 0.0f);
    }
    BoundingBox box;
    object->getBoundingBox(box);
    Log(" object max values: " << box.max.x << " " << box.max.y << " " << box.max.z << std::endl);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
    //engine->textureStore.generateCubemaps("skyboxTexture");
    engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
    engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);

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

    updateCameraPositioners(deltaSeconds);
    old_seconds = seconds;

    if (spinningBox == false && seconds > 4.0f) {
        spinningBox = true; // start spinning the logo after 4s
        spinTimeSeconds = seconds;
    }
    engine->shaders.lineShader.clearLocalLines(tr);
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
    vector<LineDef> boundingBoxes;
    for (auto& wo : engine->objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        if (spinningBox) {
            // Define a constant rotation speed (radians per second)
            double rotationSpeed = glm::radians(5.0f);
            if (!alterObjectCoords) {
                rotationSpeed = glm::radians(15.0f);
            }

            // Calculate the rotation angle based on the elapsed time
            //float rotationAngle = rotationSpeed * (seconds - spinTimeSeconds);
            float rotationAngle = rotationSpeed * deltaSeconds;

            // Apply the rotation to the modeltransform matrix
            if (doRotation) {
                //modeltransform = glm::rotate(wo->mesh->baseTransform, -rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                //if (alterObjectCoords) {
                //    modeltransform = glm::rotate(wo->mesh->baseTransform, -rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
                //}
                object->rot().y += rotationAngle;
            }
        } else {
            modeltransform = wo->mesh->baseTransform;
        }
        // standard model matrix
        auto pos = wo->pos();
        auto rot = wo->rot();
        auto scale = wo->scale();
        glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
        glm::mat4 scaled = glm::scale(mat4(1.0f), scale);
        modeltransform = trans * scaled * rotationMatrix;
        buf->model = modeltransform;
        engine->meshStore.debugRenderMeshletFromBuffers(wo, tr, modeltransform);
    }
    // lines
    engine->shaders.lineShader.prepareAddLines(tr);
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
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

    ImGui::Separator();
    // Input field for file pattern
    ImGui::InputText("File Pattern", patternBuffer, sizeof(patternBuffer));
    currentFilePattern = patternBuffer;
    if (ImGui::Button("Select GLB file from data folder")) {
        showLineSelector = true;
        ImGui::OpenPopup("Line Selector");
    }
    if (ImGui::BeginPopupModal("Line Selector", &showLineSelector)) {
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
                ImGui::CloseCurrentPopup();
            }
        }

        if (ImGui::Button("Cancel")) {
            showLineSelector = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Optionally, display the selected line
    if (selectedLine >= 0) {
        ImGui::Text("Selected: %s", files[selectedLine].c_str());
    }
    bool useAutoCameraCheckbox;
    //ImGui::Checkbox("Auto Moving Camera", &useAutoCameraCheckbox);
}
