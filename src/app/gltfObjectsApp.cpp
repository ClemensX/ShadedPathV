#include "mainheader.h"
#include "AppSupport.h"
#include "gltfObjectsApp.h"

using namespace std;
using namespace glm;

void gltfObjectsApp::run(ContinuationInfo* cont)
{
    Log("gltfObjectsApp started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        // camera initialization
        initCamera(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(0.1f);
        getHMDCameraPositioner()->setMaxSpeed(0.1f);

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.1f, 2000.0f);

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)
            .addShader(shaders.pbrShader)
            ;
        if (enableLines) shaders.addShader(shaders.lineShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("gltfObjectsApp ended" << endl);
}

void gltfObjectsApp::init() {
    engine->sound.init();
    // add some lines:
    float aspectRatio = engine->getAspect();
    float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    vector<LineDef> lines;

    // loading objects wireframe:
    //engine->objectStore.loadObject("WaterBottle.glb", "WaterBottle", lines);
    //engine->objectStore.loadMeshWireframe("small_knife_dagger/scene.gltf", "Knife", lines);

    // loading objects:
    engine->meshStore.loadMesh("WaterBottle_cmp.glb", "WaterBottle");
    engine->meshStore.loadMesh("grass.glb", "Grass");
    engine->meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
    //engine->meshStore.loadMesh("terrain_orig/Terrain_Mesh_0_0.gltf", "Knife", MeshType::MESH_TYPE_NO_TEXTURES);

    // add bottle and grass to the scene:
    engine->objectStore.createGroup("ground_group");
    grass = engine->objectStore.addObject("ground_group", "Grass.7", vec3(0.0f, 0.0f, 1000.0f));
    engine->objectStore.createGroup("knife_group");
    bottleX = engine->objectStore.addObject("knife_group", "Knife", vec3(0.3f, 0.0f, 0.0f));
    //engine->objectStore.addObject("knife_group", "WaterBottle", vec3(0.3f, 0.0f, 0.0f));
    //Log("Object loaded: " << o->id.c_str() << endl);

    bottleX->mesh->logInfo();

    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    LineShader::addZeroCross(lines);
    //LineShader::addCross(lines, vec3(1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f));

    engine->shaders.lineShader.addFixedGlobalLines(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    // load skybox cube texture
    //engine->textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    engine->textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    //engine->globalRendering.createCubeMapFrom2dTexture(engine->textureStore.BRDFLUT_TEXTURE_ID, "skyboxTexture");

    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);
    engine->textureStore.generateBRDFLUT();
    engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
    engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);


    //engine->shaders.lineShader.initialUpload();
    engine->shaders.pbrShader.initialUpload();
    // load and play music
    engine->sound.openSoundFile("power.ogg", "BACKGROUND_MUSIC", true);
    //engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 1.0f, 6000);
    // add sound to object
    engine->sound.addWorldObject(bottleX);
    engine->sound.changeSound(bottleX, "BACKGROUND_MUSIC");
    prepareWindowOutput("glTF Objects App");
    engine->presentation.startUI();
}

void gltfObjectsApp::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void gltfObjectsApp::prepareFrame(FrameResources* fr)
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

    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    // we still need to call prepareAddLines() even if we didn't actually add some
    engine->shaders.lineShader.prepareAddLines(tr);
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);

    // dynamic lines:
    plus += 0.001f;
    if (enableLines) {
        engine->shaders.lineShader.clearLocalLines(tr);
        float aspectRatio = engine->getAspect();
        LineDef myLines[] = {
            // start, end, color
            { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
            { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
            { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) }
        };
        vector<LineDef> lines;
        // add all intializer objects to vector:
        for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
        engine->shaders.lineShader.addOneTime(lines, tr);

        engine->shaders.lineShader.prepareAddLines(tr);
        engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

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
    applyViewProjection(pubo.view, pubo.proj, pubo2.view, pubo2.proj, &pubo.camPos, &pubo2.camPos);
    engine->shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine->objectStore.getGroup("knife_group");
    for (auto& wo : engine->objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        bool moveObjects = false;
        if (moveObjects) {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 10.0f), 0.0f, 0.0f));
            } else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 100.0f), 0.0f, 0.0f));
            }
        } else {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.1f, 0.0f, 0.0f));
                // test overwriting default textures used:
                //buf->indexes.baseColor = 0; // set basecolor to brdflut texture
            }
            else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 0.0f, 0.0f));
            }
        }
        // test model transforms:
        if (wo->mesh->id.starts_with("Grass")) {
            // scale to 1%:
            //modeltransform = scale(mat4(1.0f), vec3(0.01f, 0.01f, 0.01f));
            //modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1000.0f));
            // scale from gltf:
            modeltransform = wo->mesh->baseTransform;
        }
        buf->model = modeltransform;
    }
    postUpdatePerFrame(tr);
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void gltfObjectsApp::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
        engine->shaders.cubeShader.addCommandBuffers(fr, drawResult);
        if (engine->sound.enabled) {
            engine->sound.Update(camera);
        }
    }
    else if (topic == 1) {
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
    }
}

void gltfObjectsApp::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void gltfObjectsApp::processImage(FrameResources* fr)
{
    present(fr);
}

bool gltfObjectsApp::shouldClose()
{
    return shouldStopEngine;
}

void gltfObjectsApp::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}
