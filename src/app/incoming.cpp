#include "mainheader.h"
#include "AppSupport.h"
#include "incoming.h"

using namespace std;
using namespace glm;

void Incoming::run()
{
    Log("Incoming started" << endl);
    {
        setEngine(engine);
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(5.38f, 58.90f, 5.30f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(50);
        //engine.setFrameCountLimit(1000);
        setHighBackbufferResolution();
        int win_width = 1800;//480;// 960;//1800;// 800;//3700; // 2500;
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Incoming");
        //camera->saveProjectionParams(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f);
        camera->saveProjectionParams(glm::radians(45.0f), engine.getAspect(), 0.10f, 2000.0f);

        engine.registerApp(this);
        initEngine("Incoming");

        engine.textureStore.generateBRDFLUT();

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.cubeShader)
            .addShader(shaders.pbrShader)
            ;
        if (enableUI) shaders.addShader(shaders.uiShader);
        if (enableLines) shaders.addShader(shaders.lineShader);
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        eventLoop();
    }
    Log("incoming ended" << endl);
}

void Incoming::init() {
    bool terrainOnly = true; // disable all other objects
    float aspectRatio = engine.getAspect();

    // 2 square km world size
    //world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    world.setWorldSize(1024.0f, 382.0f, 1024.0f);

    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_2m.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_0.5.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.meshStore.loadMesh("incoming/valley_Mesh_0.5.glb", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.objectStore.createGroup("terrain_group");
    if (!terrainOnly) {
        engine.objectStore.createGroup("knife_group");
        engine.objectStore.createGroup("box_group");
        engine.meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
        engine.meshStore.loadMesh("box1_cmp.glb", "Box1");
        engine.meshStore.loadMesh("box10_cmp.glb", "Box10");
        engine.meshStore.loadMesh("box100_cmp.glb", "Box100");
        engine.meshStore.loadMesh("bottle2.glb", "WaterBottle");
        engine.meshStore.loadMesh("cyberpunk_pistol_cmp.glb", "Gun");
    }

    auto terrain = engine.objectStore.addObject("terrain_group", "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));
    WorldObject* knife = nullptr;
    if (!terrainOnly) {
        //auto knife = engine.objectStore.addObject("knife_group", "Knife", vec3(900.0f, 20.0f, 0.3f));
        knife = engine.objectStore.addObject("knife_group", "Knife", vec3(5.47332f, 58.312f, 3.9));
        knife->rot().x = 3.14159f / 2;
        knife->rot().y = -3.14159f / 4;
        auto gun = engine.objectStore.addObject("knife_group", "Gun", vec3(4.97f, 57.39f, 3.9));
        gun->scale() = vec3(0.03f, 0.03f, 0.03f);
        gun->rot().x = 4.8f;
        gun->rot().y = 6.4;
        gun->rot().z = 7.4f;
        worldObject = gun;
        auto bottle = engine.objectStore.addObject("knife_group", "WaterBottle", vec3(5.77332f, 58.43f, 3.6));
        auto box1 = engine.objectStore.addObject("box_group", "Box1", vec3(5.57332f, 57.3f, 3.70005));
        auto box10 = engine.objectStore.addObject("box_group", "Box10", vec3(-5.57332f, 57.3f, 3.70005));
        auto box100 = engine.objectStore.addObject("box_group", "Box100", vec3(120.57332f, 57.3f, 3.70005));
    }
    world.transformToWorld(terrain);
    auto p = hmdPositioner.getPosition();

    // heightmap
    engine.textureStore.loadTexture("valley_height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT,
        TextureFlags::KEEP_DATA_BUFFER | TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX);
    auto texHeightmap = engine.textureStore.getTexture("heightmap");
    world.setHeightmap(texHeightmap);
    unsigned int texIndexHeightmap = texHeightmap->index;
    //shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);

    // skybox
    engine.textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    engine.shaders.cubeShader.setSkybox("skyboxTexture");
    engine.shaders.cubeShader.setFarPlane(2000.0f);


    engine.shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine.shaders.pbrShader.initialUpload();
    if (enableLines) {
        if (false) {
            vector<LineDef> lines;
            terrain->addVerticesToLineList(lines, vec3(-512.0f, 0.0f, -512.0f));
            //auto verticesCount = terrain->mesh->vertices.size();
            //Log("vertices count: " << verticesCount << endl);
            engine.shaders.lineShader.addFixedGlobalLines(lines);
        }

        // Grid with 1m squares, floor on -10m, ceiling on 372m
        //Grid* grid = world.createWorldGrid(1.0f, -10.0f);
        Grid* grid = world.createWorldGrid(100.0f, 0.0f);
        // draw one line of heightmap data:
        for (int i = 0; i < world.getWorldSize().x*2; i++) {
            float v = -512.0f + (i * 0.5f);
            float w = v + 0.5f;
            vec3 p1 = vec3(v, 0.0f, v);
            //vec3 p2 = vec3(w, 0.0f, w);
            vec3 p2 = vec3(v, 0.0f, v);
            p1.y = world.getHeightmapValue(v, v);// +0.25f;
            //p2.y = world.getHeightmapValue(w, w) + 0.25f;
            p2.y = world.getHeightmapValue(v, v) + 0.25f;
            grid->lines.push_back(LineDef(p1, p2, vec4(1.0f, 1.0f, 1.0f, 1.0f)));
            //auto bug = world.getHeightmapValue(19.0f, 19.0f);
            //Log("bug: " << bug << endl);
        }
        engine.shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine.shaders.lineShader.uploadFixedGlobalLines();
    }
    // load and play music
    engine.sound.openSoundFile("power.ogg", "BACKGROUND_MUSIC", true);
    //engine.sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 1.0f, 6000);

    // add sound to object
    if (enableSound) {
        engine.sound.addWorldObject(knife);
        engine.sound.changeSound(knife, "BACKGROUND_MUSIC");
        engine.sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
    }
}

void Incoming::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void Incoming::updatePerFrame(ThreadResources& tr)
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

    updateCameraPositioners(deltaSeconds);
    //if (tr.frameNum % 100 == 0) camera->log();
    //logCameraPosition();

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
    // cube
    CubeShader::UniformBufferObject cubo{};
    CubeShader::UniformBufferObject cubo2{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(cubo.view, cubo.proj, cubo2.view, cubo2.proj);
    // reset view matrix to camera orientation without using camera position (prevent camera movin out of skybox)
    cubo.view = camera->getViewMatrixAtCameraPos();
    cubo2.view = camera->getViewMatrixAtCameraPos();
    engine.shaders.cubeShader.uploadToGPU(tr, cubo, cubo2);
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
            // all other objects
            auto pos = wo->pos();
            auto rot = wo->rot();
            auto scale = wo->scale();
            glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

            glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
            glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
            glm::mat4 scaled = glm::scale(mat4(1.0f), scale);
            modeltransform =  trans * scaled * rotationMatrix;
            //modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
            //modeltransform = wo->mesh->baseTransform;
        }
        // test model transforms:
        if (wo->mesh->id.starts_with("Grass")) {
        }
        buf->model = modeltransform;
    }
    postUpdatePerFrame(tr);
}

void Incoming::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
    if (inputState.keyEvent) {
        //Log("key pressed: " << inputState.key << endl);
        auto key = inputState.key;
        auto action = inputState.action;
        auto mods = inputState.mods;
        const bool press = action != GLFW_RELEASE;
        if (key == GLFW_KEY_X) {
            worldObject->rot().x += 0.1f / 4;
            Log("rot x " << worldObject->rot().x << endl);
        }
        if (key == GLFW_KEY_Y) {
            worldObject->rot().y += 0.1f / 4;
            Log("rot y " << worldObject->rot().y << endl);
        }
        if (key == GLFW_KEY_Z) {
            worldObject->rot().z += 0.1f / 4;
            Log("rot z " << worldObject->rot().z << endl);
        }
        if (key == GLFW_KEY_D) {
        }
        if (key == GLFW_KEY_1) {
        }
        if (key == GLFW_KEY_2) {
        }
        if (mods & GLFW_MOD_SHIFT) {
        }
        if (key == GLFW_KEY_SPACE) {
        }
    }
}

void Incoming::buildCustomUI()
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
