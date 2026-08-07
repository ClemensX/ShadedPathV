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
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    PBRShader::LightSource ls;
    ls.color = vec3(1.0f);
    ls.position = vec3(75.0f, 0.5f, -20.0f);

    // new
    GPUFrameParam frameParam;
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
    if (ImGui::Button("Environment Cube Settings")) {
        displayParams.showEnvCubeDialog = true;
        ImGui::OpenPopup("EnvCubeSettings");
    }
    if (ImGui::BeginPopupModal("EnvCubeSettings", &displayParams.showEnvCubeDialog)) {
        ImGui::Text("42");
        ImGui::EndPopup();
    }
}