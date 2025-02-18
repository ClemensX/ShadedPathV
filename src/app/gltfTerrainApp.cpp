#include "mainheader.h"
#include "AppSupport.h"
#include "gltfTerrainApp.h"

using namespace std;
using namespace glm;

void gltfTerrainApp::run(ContinuationInfo* cont)
{
    Log("gltfTerrainApp started" << endl);
    {
        AppSupport::setEngine(engine);
        Shaders& shaders = engine->shaders;
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(0.0f, 70.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
        initCamera();
        auto p = getHMDCameraPositioner()->getPosition();
        //hmdPositioner->setPosition(glm::vec3(900.0f, p.y, 1.0f));
        getHMDCameraPositioner()->setPosition(glm::vec3(5.38f, 58.90f, 5.30f));
        p = getHMDCameraPositioner()->getPosition();
        Log("HMD position: " << p.x << " / " << p.y << " / " << p.z << endl);
        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        //engine->setMaxTextures(50);
        //engine->setFrameCountLimit(1000);
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.10f, 2000.0f);

        engine->textureStore.generateBRDFLUT();

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.pbrShader)
            ;
        if (enableUI) shaders.addShader(shaders.uiShader);
        if (enableLines) shaders.addShader(shaders.lineShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("gltfTerrainApp ended" << endl);
}

void gltfTerrainApp::init() {
    float aspectRatio = engine->getAspect();

    // 2 square km world size
    //world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    world.setWorldSize(1024.0f, 382.0f, 1024.0f);

    //engine->meshStore.loadMesh("terrain2k/Project_Mesh_2m.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine->meshStore.loadMesh("terrain2k/Project_Mesh_0.5.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine->meshStore.loadMesh("incoming/valley_Mesh_0.5.glb", "WorldBaseTerrain", MeshFlagsCollection(MeshFlags::MESH_TYPE_NO_TEXTURES));
    engine->objectStore.createGroup("terrain_group");
    engine->objectStore.createGroup("knife_group");
    engine->objectStore.createGroup("box_group");
    engine->meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
    engine->meshStore.loadMesh("box1_cmp.glb", "Box1");
    engine->meshStore.loadMesh("box10_cmp.glb", "Box10");
    engine->meshStore.loadMesh("box100_cmp.glb", "Box100");
    engine->meshStore.loadMesh("bottle2.glb", "WaterBottle");

    auto terrain = engine->objectStore.addObject("terrain_group", "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));
    //auto knife = engine->objectStore.addObject("knife_group", "Knife", vec3(900.0f, 20.0f, 0.3f));
    auto knife = engine->objectStore.addObject("knife_group", "Knife", vec3(5.47332f, 58.312f, 3.9));
    knife->rot().x = 3.14159f / 2;
    knife->rot().y = -3.14159f / 4;
    auto bottle = engine->objectStore.addObject("knife_group", "WaterBottle", vec3(5.77332f, 58.43f, 3.6));
    auto box1 = engine->objectStore.addObject("box_group", "Box1", vec3(5.57332f, 57.3f, 3.70005));
    auto box10 = engine->objectStore.addObject("box_group", "Box10", vec3(-5.57332f, 57.3f, 3.70005));
    auto box100 = engine->objectStore.addObject("box_group", "Box100", vec3(120.57332f, 57.3f, 3.70005));
    world.transformToWorld(terrain);
    auto p = hmdPositioner.getPosition();

    engine->shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine->shaders.pbrShader.initialUpload();
    if (enableLines) {
        // Grid with 1m squares, floor on -10m, ceiling on 372m
        //Grid* grid = world.createWorldGrid(1.0f, -10.0f);
        Grid* grid = world.createWorldGrid(100.0f, 0.0f);
        engine->shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine->shaders.lineShader.uploadFixedGlobalLines();
    }
    // load and play music
    //engine->sound.openSoundFile("power.ogg", "BACKGROUND_MUSIC", true);
    ////engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 1.0f, 6000);
    //// add sound to object
    //engine->sound.addWorldObject(knife);
    //engine->sound.changeSound(knife, "BACKGROUND_MUSIC");
    //engine->sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
    prepareWindowOutput("Line App");
    engine->presentation.startUI();
}

void gltfTerrainApp::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void gltfTerrainApp::prepareFrame(FrameResources* fr)
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
    if (enableLines) {
        LineShader::UniformBufferObject lubo{};
        LineShader::UniformBufferObject lubo2{};
        lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
        engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
    }
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
        if (wo->objectNum == 0) {
            //terrain
            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1f, 0.0f, 0.0f));
            // test overwriting default textures used:
            //buf->indexes.baseColor = 0; // set basecolor to brdflut texture
            uiVerticesTotal = wo->mesh->vertices.size();
            uiVerticesSqrt = (unsigned long)sqrt(uiVerticesTotal);
            // scale to 400%:
            //modeltransform = scale(mat4(1.0f), vec3(4.01f, 4.01f, 4.01f));
            // scale from gltf:
            modeltransform = wo->mesh->baseTransform;

        }
        else {
            // knife
            auto pos = wo->pos();
            auto rot = wo->rot();
            glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

            glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;

            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z)) * rotationMatrix;
            //modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
            //modeltransform = wo->mesh->baseTransform;
        }
        // test model transforms:
        if (wo->mesh->id.starts_with("Grass")) {
        }
        buf->model = modeltransform;
    }
    postUpdatePerFrame(tr);
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void gltfTerrainApp::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        // draw lines and objects
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
    } else if (topic == 1) {
        engine->shaders.pbrShader.addCommandBuffers(fr, drawResult);
    }
}

void gltfTerrainApp::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void gltfTerrainApp::processImage(FrameResources* fr)
{
    present(fr);
}

bool gltfTerrainApp::shouldClose()
{
    return shouldStopEngine;
}

void gltfTerrainApp::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}

void gltfTerrainApp::buildCustomUI()
{
    ImGui::Separator();
    int sizeX = world.getWorldSize().x;
    ImGui::Text("World size [m]: %d * %d", sizeX, sizeX);
    ImGui::Separator();
    double time = ThemedTimer::getInstance()->getLatestTiming(TIMER_PART_GLOBAL_UPDATE);
    ImGui::Text("Terrain vertices total: %d , ( %d * %d)", uiVerticesTotal, uiVerticesSqrt, uiVerticesSqrt);
    ImGui::Separator();
    float resolution = (float)sizeX / (float)uiVerticesSqrt;
    ImGui::Text("Terrain resolution: %f [m]", resolution);
    ImGui::Separator();
};
