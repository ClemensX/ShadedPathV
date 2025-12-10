#include "mainheader.h"
#include "AppSupport.h"
#include "Forest.h"

using namespace std;
using namespace glm;

void Forest::run(ContinuationInfo* cont)
{
    Log("Forest started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        engine->appname = "Forest";
        // camera initialization
        initCamera(glm::vec3(-0.640809f, -0.445347f, 2.82217f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        Movement mv;
        camera->setConstantSpeed(mv.fallSpeedMS);
        //camera->setConstantSpeed(mv.runSpeedMS);
        //camera->setConstantSpeed(mv.walkSpeedMS);
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
    Log("Forest ended" << endl);
}

void Forest::init() {
    engine->sound.init(false);

    engine->textureStore.generateBRDFLUT();

    engine->objectStore.loadWorldCreatorInstances("ObjectTest_InstanceInfo.json");
    MeshFlagsCollection meshFlags;
    //meshFlags.setFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER);
    //meshFlags.setFlag(MeshFlags::MESHLET_DEBUG_COLORS);
    //engine->meshStore.loadMesh("terrain_forest_small_cmp.glb", "LogoBox", meshFlags);
    engine->meshStore.loadMesh("ObjectTest_cmp.glb", "LogoBox", meshFlags);
    //engine->meshStore.loadMesh("terrain_forest_cmp.glb", "LogoBox", meshFlags);// alterObjectCoords = true;

    engine->objectStore.createGroup("group");
    //object = engine->objectStore.addObject("group", "LogoBox", vec3(0.0f, 14.38f * 2.5f, 0.0f));
    object = engine->objectStore.addObject("group", "LogoBox", vec3(0.0f, 0.0f, 0.0f));

    engine->meshStore.loadMesh("box1_cmp.glb", "Flora_1", meshFlags);
    engine->objectStore.createGroup("flora");
    auto wc = engine->objectStore.getWorldCreator();
    for (const auto& biomeObject : wc->biomeObjects) {
        const auto& merged = biomeObject.MergedParsedTile;
        if (merged.has_value()) {
            for (const auto& instance : merged->instances) {
                float y = instance.t.y / 1024.0f;
                // check height within margin around 0.017788842

                if (epsilonEqual(y, (float)0.017788842, 0.0000001f)) {
                    static int count = 0;
                    Log("YEAHHHHHHHH! " << ++count << " " << y << endl);
                }

                //vec3 pos = vec3(256.0f - instance.t.x, 2.5f * instance.t.y, instance.t.z - 256.0f);
                vec3 pos = vec3(-1.0f * (instance.t.z - 256.0f), 2.5f * instance.t.y, -1.0f * (256.0f - instance.t.x));
                pos.x = pos.x * -1.0f + 256;
                // stretch terrain in y direction
                float ystretch = (pos.y / 2.5f) - 14.3864f;
                ystretch *= 10.0f;
                pos.y = 2.5 * (ystretch + 14.3864f);

                pos = vec3(instance.t.x, instance.t.y, -instance.t.z);
                Log("Instance position: " << pos.x << " " << pos.y << " " << pos.z << std::endl);
                auto obj = engine->objectStore.addObject("flora", "Flora_1", pos);
                //obj->rot() = instance.rotation;
                //float scale = instance.scale.x; // uniform scale
                //obj->scale() = vec3(scale);
            }
        }
    }

    object->enableDebugGraphics = false;
    if (alterObjectCoords) {
        // turn upside down
        object->rot() = vec3(PI_half, 0.0, 0.0f);
    }
    BoundingBox box;
    object->getBoundingBoxWorld(box, mat4(1.0f));
    Log(" object max values: " << box.max.x << " " << box.max.y << " " << box.max.z << std::endl);
    float scale = 1.0f;
    if (false) {
        // scale to have 1m cube diameter for LOD 0 object:
        float diameter = length(box.max - box.min);
        scale = 1.732f / diameter;
        //scale *= 12.0f;
        scale = 1.0f;
        object->scale() = vec3(scale);
        object->enabled = true;
        object->useGpuLod = true;
        object->enableDebugGraphics = false;
    }

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    bool generateCubemaps = false;
    // transform terrain to world size
    //engine->textureStore.loadTexture("nebula.ktx2", "skyboxTexture");
    engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    if (generateCubemaps) {
        // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
        engine->textureStore.generateCubemaps("skyboxTexture");
    } else {
        engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
        engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);
    }

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);

    PBRShader::LightSource ls;
    ls.color = vec3(1.0f);
    ls.position = vec3(75.0f, 30.5f, -40.0f);
    engine->shaders.pbrShader.changeLightSource(ls.color, ls.position);
    engine->shaders.pbrShader.initialUpload();
    // window creation
    prepareWindowOutput("Forest");
    engine->presentation.startUI();

    // load and play music
    engine->sound.openSoundFile(Sound::SHADED_PATH_JINGLE_FILE, Sound::SHADED_PATH_JINGLE);
    engine->sound.playSound(Sound::SHADED_PATH_JINGLE, SoundCategory::MUSIC);
    engine->sound.openSoundFile("loading_music.ogg", "BACKGROUND_MUSIC", true);
    engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 0.2f, 5000);

    vector<LineDef> lines;
    engine->shaders.lineShader.addZeroCross(lines);
    engine->shaders.lineShader.addFixedGlobalLines(lines);
    engine->shaders.lineShader.uploadFixedGlobalLines();
}

void Forest::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void Forest::prepareFrame(FrameResources* fr)
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
    if (seconds > 20.0f) {
        //object->enabled = false;
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

    // we only need to update dynamic model UBOs for first few frames, afterwards they remain static
    if (tr.frameNum < 4) {
        for (auto& wo : engine->objectStore.getSortedList()) {
            //Log(" adapt object " << obj.get()->objectNum << endl);
            //WorldObject *wo = obj.get();
            PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
            // standard model matrix
            mat4 modeltransform;
            wo->calculateStandardModelTransform(modeltransform);
            buf->model = modeltransform;
            buf->params[0].intensity = 7.0f; // adjust sun light intensity
            //if (wo->objectNum == 1) {
            //    Log(" object 1 pos: " << wo->pos().x << " " << wo->pos().y << " " << wo->pos().z << endl);
            //    Log("   bb in buf: " << buf->boundingBox.min.x << " " << buf->boundingBox.min.y << " " << buf->boundingBox.min.z << " - "
            //        << buf->boundingBox.max.x << " " << buf->boundingBox.max.y << " " << buf->boundingBox.max.z << endl);
            //}
            //buf->boundingBox = wo->perFrameBB;
            if (!object->enabled)   buf->disableRendering();
            if (wo->enableDebugGraphics) engine->meshStore.debugGraphics(wo, tr, modeltransform, true, false, false, false);
        }
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
void Forest::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
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
        //Log("Forest::drawFrame: PBR shader command buffers added" << endl);
    }
}

void Forest::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void Forest::processImage(FrameResources* fr)
{
    present(fr);
}

bool Forest::shouldClose()
{
    return shouldStopEngine;
}

void Forest::handleInput(InputState& inputState)
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