#include "mainheader.h"
#include "AppSupport.h"
#include "Loader.h"

using namespace std;
using namespace glm;

void Loader::run(ContinuationInfo* cont)
{
    Log("Loader started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        // camera initialization
        //initCamera(glm::vec3([-0.0386716 0.57298 1.71695]), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        initCamera(glm::vec3(-0.0386716f,  0.5f, 1.71695f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);
        getHMDCameraPositioner()->setMaxSpeed(0.1f);

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.1f, 2000.0f);

        engine->textureStore.generateBRDFLUT();

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)
            .addShader(shaders.pbrShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("Loader ended" << endl);
}

void Loader::init() {
    engine->sound.init(false);

    //engine->meshStore.loadMesh("loadingbox_cmp.glb", "LogoBox");
    engine->meshStore.loadMesh("DamagedHelmet_cmp.glb", "LogoBox");
    engine->objectStore.createGroup("group");
    bottle = engine->objectStore.addObject("group", "LogoBox", vec3(0.0f, 0.0f, 0.0f));

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    // load skybox cube texture
    engine->textureStore.loadTexture("nebula.ktx2", "skyboxTexture");

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);


    engine->shaders.pbrShader.initialUpload();
    // window creation
    prepareWindowOutput("Loader (insert correct app name here)");
    engine->presentation.startUI();

    // load and play music
    engine->sound.openSoundFile(Sound::SHADED_PATH_JINGLE_FILE, Sound::SHADED_PATH_JINGLE);
    engine->sound.playSound(Sound::SHADED_PATH_JINGLE, SoundCategory::MUSIC);
    engine->sound.openSoundFile("loading_music.ogg", "BACKGROUND_MUSIC", true);
    engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 0.2f, 5000);
}

void Loader::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void Loader::prepareFrame(FrameResources* fr)
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
    applyViewProjection(pubo.view, pubo.proj, pubo2.view, pubo2.proj);
    engine->shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine->objectStore.getGroup("knife_group");
    for (auto& wo : engine->objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicUniformBufferObject* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        if (spinningBox) {
            // Define a constant rotation speed (radians per second)
            const double rotationSpeed = glm::radians(45.0f); // 45 degrees per second

            // Calculate the rotation angle based on the elapsed time
            float rotationAngle = rotationSpeed * (seconds - spinTimeSeconds);

            // Apply the rotation to the modeltransform matrix
            modeltransform = glm::rotate(wo->mesh->baseTransform, -rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            modeltransform = wo->mesh->baseTransform;
        }
        buf->model = modeltransform;
    }
    postUpdatePerFrame(tr);
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void Loader::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        //engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
        if (engine->sound.enabled) {
            engine->sound.Update(camera);
        }
    }
    else if (topic == 1) {
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
    }
}

void Loader::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void Loader::processImage(FrameResources* fr)
{
    present(fr);
}

bool Loader::shouldClose()
{
    return shouldStopEngine;
}

void Loader::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}
