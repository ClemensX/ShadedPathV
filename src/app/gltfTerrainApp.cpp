#include "mainheader.h"
#include "gltfTerrainApp.h"

using namespace std;
using namespace glm;

void gltfTerrainApp::run()
{
    Log("gltfTerrainApp started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        CameraPositioner_HMD myhmdPositioner(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        hmdPositioner = &myhmdPositioner;
        positioner.setMaxSpeed(15.0f);
        Camera camera(&engine);
        hmdPositioner->setCamera(&camera);
        camera.changePositioner(positioner);
        //camera.changePositioner(*hmdPositioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        engine.enableVR();
        engine.enableStereo();
        engine.enableStereoPresentation();
        engine.setFixedPhysicalDeviceIndex(0); // needed for Renderdoc
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(50);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK); // 960
        int win_width = 800;//480;// 960;//1800;// 800;//3700; // 2500;
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Render glTF terrain");
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f));

        engine.setFramesInFlight(2);
        engine.registerApp(this);
        if (engine.isVR()) {
            engine.vr.SetPositioner(hmdPositioner);
        }
        //engine.enableSound();
        engine.setThreadModeSingle();

        // engine initialization
        engine.init("gltfTerrain");

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

    //engine.meshStore.loadMesh("terrain_cmp.glb", "WorldBaseTerrain");
    //engine.meshStore.loadMesh("terrain_orig/Terrain_Mesh_0_0.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine.meshStore.loadMesh("terrain_vh/Project_Mesh_1_1.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.meshStore.loadMesh("terrain2k/Project_Mesh_2m.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_0.5.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.objectStore.createGroup("terrain_group");
    auto terrain = engine.objectStore.addObject("terrain_group", "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));
    world.transformToWorld(terrain);

    engine.shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine.shaders.pbrShader.initialUpload();
    if (enableLines) {
        // Grid with 1m squares, floor on -10m, ceiling on 372m
        //Grid* grid = world.createWorldGrid(1.0f, -10.0f);
        Grid* grid = world.createWorldGrid(1.0f, 0.0f);
        engine.shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine.shaders.lineShader.uploadFixedGlobalLines();
    }
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
        return;
    }
    if (old_seconds > seconds) {
        Log("INVERTED TIME" << endl);
        return;
    }
    double deltaSeconds = seconds - old_seconds;
    positioner->update(deltaSeconds, input.pos, input.pressedLeft);
    old_seconds = seconds;
    // lines
    if (enableLines) {
        LineShader::UniformBufferObject lubo{};
        lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        lubo.view = camera->getViewMatrix();
        lubo.proj = camera->getProjectionNDC();
        engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo);
    }
    // pbr
    PBRShader::UniformBufferObject pubo{};
    PBRShader::UniformBufferObject pubo2{};
    //mat4 modeltransform = glm::mat4(1.0f); //glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    mat4 modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    if (engine.isVR()) {
        pubo.model = modeltransform;
        pubo.view = hmdPositioner->getViewMatrixLeft();
        pubo.proj = hmdPositioner->getProjectionLeft();
        pubo2.model = modeltransform;
        pubo2.view = hmdPositioner->getViewMatrixRight();
        pubo2.proj = hmdPositioner->getProjectionRight();
    }
    else {
        pubo.model = modeltransform;
        pubo.view = camera->getViewMatrix();
        pubo.proj = camera->getProjectionNDC();
        //pubo.model[0][0] = -1.0f;    
        pubo2.model = modeltransform;
        pubo2.view = camera->getViewMatrix();
        pubo2.proj = camera->getProjectionNDC();
        //pubo2.model[0][0] = -1.0f;
    }
    engine.shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine.objectStore.getGroup("knife_group");
    for (auto& wo : engine.objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicUniformBufferObject* buf = engine.shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        if (wo->objectNum == 0) {
            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1f, 0.0f, 0.0f));
            // test overwriting default textures used:
            //buf->indexes.baseColor = 0; // set basecolor to brdflut texture
            modeltransform = wo->mesh->baseTransform;
            uiVerticesTotal = wo->mesh->vertices.size();
            uiVerticesSqrt = (unsigned long)sqrt(uiVerticesTotal);
        } else {
            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 0.0f, 0.0f));
        }
        // test model transforms:
        if (wo->mesh->id.starts_with("Grass")) {
            // scale to 1%:
            //modeltransform = scale(mat4(1.0f), vec3(0.01f, 0.01f, 0.01f));
            // scale from gltf:
            modeltransform = wo->mesh->baseTransform;
        }
        buf->model = modeltransform;
        if (engine.isDedicatedRenderUpdateThread(tr)) {
            engine.sound.Update(camera);
        }
    }
}

void gltfTerrainApp::handleInput(InputState& inputState)
{
    if (inputState.mouseButtonEvent) {
        //Log("mouse button pressed (left/right): " << inputState.pressedLeft << " / " << inputState.pressedRight << endl);
        input.pressedLeft = inputState.pressedLeft;
        input.pressedRight = inputState.pressedRight;
    }
    if (inputState.mouseMoveEvent) {
        //Log("mouse pos (x/y): " << inputState.pos.x << " / " << inputState.pos.y << endl);
        input.pos.x = inputState.pos.x;
        input.pos.y = inputState.pos.y;
    }
    if (inputState.keyEvent) {
        //Log("key pressed: " << inputState.key << endl);
        auto key = inputState.key;
        auto action = inputState.action;
        auto mods = inputState.mods;
        const bool press = action != GLFW_RELEASE;
        if (key == GLFW_KEY_W)
            positioner->movement.forward_ = press;
        if (key == GLFW_KEY_S)
            positioner->movement.backward_ = press;
        if (key == GLFW_KEY_A)
            positioner->movement.left_ = press;
        if (key == GLFW_KEY_D)
            positioner->movement.right_ = press;
        if (key == GLFW_KEY_1)
            positioner->movement.up_ = press;
        if (key == GLFW_KEY_2)
            positioner->movement.down_ = press;
        if (mods & GLFW_MOD_SHIFT)
            positioner->movement.fastSpeed_ = press;
        if (key == GLFW_KEY_SPACE)
            positioner->setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
    }
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
