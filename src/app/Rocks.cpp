#include "mainheader.h"
#include "AppSupport.h"
#include "Rocks.h"

using namespace std;
using namespace glm;

void Rocks::run(ContinuationInfo* cont)
{
    Log("Rocks started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        engine->appname = "Tech Demo 1";
        // camera initialization
        initCamera(glm::vec3(-0.640809f, -0.445347f, 2.82217f), glm::vec3(0.0f, 0.5f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);
        //getFirstPersonCameraPositioner()->setMaxSpeed(200.0f);
        getHMDCameraPositioner()->setMaxSpeed(0.1f);
        Movement mv;
        //camera->setConstantSpeed(mv.fallSpeedMS);
        //camera->setConstantSpeed(mv.runSpeedMS);
        camera->setConstantSpeed(mv.walkSpeedMS);
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
    Log("Rocks ended" << endl);
}

void redoBB(WorldObject* obj) {
    mat4 modeltransform = mat4(0.0f);
    BoundingBox box;
    obj->getBoundingBox(box); // get the unscaled mesh BB
    Util::calculateStandardModelTransform(modeltransform, obj->pos(), obj->scale(), obj->rot());
    Util::recalculateBoundingBox(modeltransform, box);
    obj->perFrameBB = box;
}
void Rocks::addRandomRocks(std::vector<WorldObject*>& rocks, World& world, MeshInfo* meshInfo, float scale1m) {

    unsigned long total_rocks = 10000;
    if (engine->isVR()) total_rocks /= 4; // reduce number of rocks for VR mode for performance reasons

    // create randomly positioned billboards for each vacXX texture we have:
    for (unsigned long num = 0; num < total_rocks; num++) {
        vec3 rnd = world.getRandomPos();
        vec3 pos;
        float denseFactor = 8.0f;
        if (engine->isVR()) denseFactor *= 2.0f; // reduce density for VR mode for performance reasons
        pos.x = rnd.x/denseFactor;
        pos.y = rnd.y/denseFactor;
        pos.z = rnd.z/denseFactor;
        //pos.x = num * 1.0f;
        //pos.y = 0.0f;
        //pos.z = -300.0f;

        WorldObject* rock = engine->objectStore.addObject("group", "LogoBox", pos);

        // scale between 1 and 10 meters:
        //float scale = MathHelper::RandF(scale1m, 10.0f * scale1m);
        float scale = MathHelper::RandF(1.0f, 3.0f);
        scale = 1.0f;// scale1m;
        scale = scale1m;
        rock->scale() = vec3(scale);
        rock->enabled = true;
        rock->useGpuLod = true;
        vec3 rotation;
        rotation.x = MathHelper::RandF(0.0f, PI);
        rotation.y = MathHelper::RandF(0.0f, PI);
        rotation.z = MathHelper::RandF(0.0f, PI);
        //rock->rot() = vec3(PI_half, 0.0, 0.0f);
        rock->rot() = rotation;
        //mat4 modeltransform = mat4(0.0f);
        //BoundingBox box;
        //rock->getBoundingBox(box); // get the unscaled mesh BB
        //Util::calculateStandardModelTransform(modeltransform, rock->pos(), rock->scale(), rock->rot());
        //Util::recalculateBoundingBox(modeltransform, box);
        //rock->perFrameBB = box;
        auto box = rock->perFrameBB;
        redoBB(rock);
        float width = box.max.x - box.min.x;
        scale = 1.0f / width;
        //Log("add rocks: width " << width << " scale " << scale << endl);
        rocks.push_back(rock);
    }
}

void Rocks::init() {
    engine->sound.init(false);

    engine->textureStore.generateBRDFLUT();

    MeshFlagsCollection meshFlags;
    //meshFlags.setFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER);
    //meshFlags.setFlag(MeshFlags::MESHLET_DEBUG_COLORS);
    engine->meshStore.loadMesh("granite_rock_lod_cmp.glb", "LogoBox", meshFlags); alterObjectCoords = true;

    engine->objectStore.createGroup("group");
    object = engine->objectStore.addObject("group", "LogoBox", vec3(-0.2f, 0.2f, 0.2f));

    object->enableDebugGraphics = false;
    if (alterObjectCoords) {
        // turn upside down
        object->rot() = vec3(PI_half, 0.0, 0.0f);
    }
    BoundingBox box;
    object->getBoundingBox(box);
    Log(" object max values: " << box.max.x << " " << box.max.y << " " << box.max.z << std::endl);
    // scale to have 1m width for LOD 0 object:
    float width = box.max.x - box.min.x;
    float scale = 1.0f / width;
    if (true) {
        object->scale() = vec3(scale);
        object->enabled = true;
        object->useGpuLod = true;
        redoBB(object);
    }

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    bool generateCubemaps = false;
    // transform terrain to world size
    engine->textureStore.loadTexture("nebula.ktx2", "skyboxTexture");
    //engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    if (generateCubemaps) {
        // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
        engine->textureStore.generateCubemaps("skyboxTexture");
    } else {
        engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
        engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);
    }

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);

    vector<WorldObject*> rocks;
    addRandomRocks(rocks, world, object->mesh, scale);

    PBRShader::LightSource ls;
    ls.color = vec3(1.0f);
    ls.position = vec3(75.0f, -10.5f, -40.0f);
    engine->shaders.pbrShader.changeLightSource(ls.color, ls.position);
    engine->shaders.pbrShader.initialUpload();
    // window creation
    prepareWindowOutput("Rocks");
    engine->presentation.startUI();

    // load and play music
    engine->sound.openSoundFile(Sound::SHADED_PATH_JINGLE_FILE, Sound::SHADED_PATH_JINGLE);
    engine->sound.playSound(Sound::SHADED_PATH_JINGLE, SoundCategory::MUSIC);
    engine->sound.openSoundFile("loading_music.ogg", "BACKGROUND_MUSIC", true);
    engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 0.2f, 5000);

}

void Rocks::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void Rocks::prepareFrame(FrameResources* fr)
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
        Util::calculateStandardModelTransform(modeltransform, wo->pos(), wo->scale(), wo->rot());
        buf->model = modeltransform;
        buf->params[0].intensity = 10.0f; // adjust sun light intensity
        //buf->boundingBox = wo->perFrameBB;
        if (!object->enabled)   buf->disableRendering();
        if (wo->enableDebugGraphics) engine->meshStore.debugGraphics(wo, tr, modeltransform, false, true, false, false);
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
void Rocks::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
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
        //Log("Rocks::drawFrame: PBR shader command buffers added" << endl);
    }
}

void Rocks::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void Rocks::processImage(FrameResources* fr)
{
    present(fr);
}

bool Rocks::shouldClose()
{
    return shouldStopEngine;
}

void Rocks::handleInput(InputState& inputState)
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