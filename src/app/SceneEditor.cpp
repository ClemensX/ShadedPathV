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
        vec3 cameraPosition(0, 0, 0);
        initCamera(cameraPosition, glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Movement mv;
        camera->setConstantSpeed(mv.runSpeedMS * 8);
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
        MStore& mstore = engine->mstore;
        //engine->mstore.loadMesh("test/cube_single.gltf", "SingleMesh", flags);
        engine->mstore.loadMesh("DamagedHelmet_cmp.glb", "SingleMesh", flags);
        auto loaded = mstore.getMeshFileByID("SingleMesh"); // ensure we can retrieve the mesh file by ID
        auto meshInfo = mstore.getGPUMeshInfo(loaded->meshes[0].meshIndex);
        const auto meshMetadata = mstore.getMeshMetadata(loaded->meshes[0].meshIndex);
        SceneObject* object = engine->mstore.addObject(meshInfo->index, vec3(0.0f, 0.0f, 0.0f));
            // turn upside down
            object->rot = vec3(PI_half, 0.0, 0.0f);
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
        displayParams.filePattern = ".ktx2";
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