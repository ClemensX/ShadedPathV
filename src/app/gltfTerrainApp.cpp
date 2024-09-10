#include "mainheader.h"
#include "AppSupport.h"
#include "gltfTerrainApp.h"

using namespace std;
using namespace glm;

void gltfTerrainApp::run()
{
    Log("gltfTerrainApp started" << endl);
    {
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(0.0f, 20.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
        initCamera(engine);
        auto p = getHMDCameraPositioner()->getPosition();
        //hmdPositioner->setPosition(glm::vec3(900.0f, p.y, 1.0f));
        getHMDCameraPositioner()->setPosition(glm::vec3(900.0f, 20.0f, 1.0f));
        p = getHMDCameraPositioner()->getPosition();
        Log("HMD position: " << p.x << " / " << p.y << " / " << p.z << endl);
        //this->camera = &camera;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        if (vr) {
            engine.enableVR();
        }
        engine.enableStereo();
        engine.enableStereoPresentation();
        engine.setFixedPhysicalDeviceIndex(0); // needed for Renderdoc
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(50);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::HMDIndex);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK); // 960
        int win_width = 800;//480;// 960;//1800;// 800;//3700; // 2500;
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Render glTF terrain");
        camera->saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f));

        engine.registerApp(this);
        if (engine.isVR()) {
            engine.vr.SetPositioner(getHMDCameraPositioner());
            engine.setFramesInFlight(1);
        } else {
            engine.setFramesInFlight(2);
        }
        //engine.enableSound();
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init("gltfTerrain");
        // even if we wanted VR initialization may have failed, fallback to non-VR
        if (!engine.isVR()) {
            camera->changePositioner(fpPositioner);
        }

        engine.textureStore.generateBRDFLUT();
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

        // some shaders may need additional preparation
        engine.prepareDrawing();


        // rendering
        while (!engine.shouldClose()) {
            engine.pollEvents();
            engine.drawFrame();
        }
        engine.waitUntilShutdown();
    }
    Log("gltfTerrainApp ended" << endl);
}

void gltfTerrainApp::init() {
    float aspectRatio = engine.getAspect();

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);

    engine.meshStore.loadMesh("terrain2k/Project_Mesh_2m.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_0.5.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.objectStore.createGroup("terrain_group");
    engine.objectStore.createGroup("knife_group");
    engine.meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
    auto terrain = engine.objectStore.addObject("terrain_group", "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));
    auto knife = engine.objectStore.addObject("knife_group", "Knife", vec3(900.0f, 20.0f, 0.3f));
    world.transformToWorld(terrain);
    auto p = hmdPositioner.getPosition();

    engine.shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine.shaders.pbrShader.initialUpload();
    if (enableLines) {
        // Grid with 1m squares, floor on -10m, ceiling on 372m
        //Grid* grid = world.createWorldGrid(1.0f, -10.0f);
        Grid* grid = world.createWorldGrid(1.0f, 0.0f);
        engine.shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine.shaders.lineShader.uploadFixedGlobalLines();
    }
    // load and play music
    engine.sound.openSoundFile("power.ogg", "BACKGROUND_MUSIC", true);
    //engine.sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 1.0f, 6000);
    // add sound to object
    engine.sound.addWorldObject(knife);
    engine.sound.changeSound(knife, "BACKGROUND_MUSIC");
    engine.sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
}

void gltfTerrainApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void gltfTerrainApp::updatePerFrame(ThreadResources& tr)
{
    static double old_seconds = 0.0f;
    double seconds = engine.gameTime.getTimeSeconds();
    if (old_seconds > 0.0f && old_seconds == seconds) {
        Log("DOUBLE TIME" << endl);
        //tr.discardFrame = true;
        return;
    }
    if (old_seconds > seconds) {
        Log("INVERTED TIME" << endl);
        //tr.discardFrame = true;
        return;
    }
    double deltaSeconds = seconds - old_seconds;
    //engine.presentation.beginPresentFrame(tr);
    //engine.vr.frameBegin(tr);
    fpPositioner.update(deltaSeconds, input.pos, input.pressedLeft);
    hmdPositioner.updateDeltaSeconds(deltaSeconds);
    old_seconds = seconds;
    // lines
    if (enableLines) {
        LineShader::UniformBufferObject lubo{};
        LineShader::UniformBufferObject lubo2{};
        lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
        engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
    }
    // pbr
    PBRShader::UniformBufferObject pubo{};
    PBRShader::UniformBufferObject pubo2{};
    mat4 modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    pubo.model = modeltransform;
    pubo2.model = modeltransform;
    applyViewProjection(pubo.view, pubo.proj, pubo2.view, pubo2.proj);
    engine.shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine.objectStore.getGroup("knife_group");
    for (auto& wo : engine.objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicUniformBufferObject* buf = engine.shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
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
            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
            //modeltransform = wo->mesh->baseTransform;
        }
        // test model transforms:
        if (wo->mesh->id.starts_with("Grass")) {
        }
        buf->model = modeltransform;
        if (engine.isDedicatedRenderUpdateThread(tr)) {
            engine.sound.Update(camera);
        }
    }
}

void gltfTerrainApp::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
    return;
}

void gltfTerrainApp::buildCustomUI()
{
    static string helpText =
        "g generate new seed\n"
        "+ next Generation\n"
        "- previous Generation\n"
        "h write heightmap to file (VK_FORMAT_R32_SFLOAT)\n"
        "p dump image";
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
