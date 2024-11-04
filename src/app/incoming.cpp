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
        vec3 camStart(5.38f, 58.7576f, 5.30f);
        //vec3 camStart(-511.00f, 358.90f, -511.00f);
        //vec3 camStart(5.38f, -458.90f, 5.30f);
        //vec3 camStart(0.00f, 358.90f, 0.00f);
        initCamera(camStart, vec3(0.0f, 50.0f, -100.0f), vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
        auto p = getHMDCameraPositioner()->getPosition();
        Log("HMD position: " << p.x << " / " << p.y << " / " << p.z << endl);
        if (enableIntersectTest) {
            intersectTestLine.color = Colors::Red;
            intersectTestLine.start = camStart + vec3(0.5f, -0.3f, 0.0f);
            intersectTestLine.end = camStart + vec3(0.5f, -0.3f, -1.0f);
        }
        // engine configuration
        enableEventsAndModes();
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(50);
        //engine.setFrameCountLimit(1000);
        setHighBackbufferResolution();
        int win_width = 2500;//480;// 960;//1800;// 800;//3700; // 2500;
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

void Incoming::addRandomHeightLines(vector<LineDef>& lines, World& world) {
    LineDef b;
    //unsigned long total_billboards = 50000000;
    //unsigned long total_billboards = 1000000;
    //unsigned long total_billboards = 500000;
    //unsigned long total_billboards = 200000;
    //unsigned long total_billboards = 5000;
    unsigned long total_billboards = 12;

    // create randomly positioned billboards for each vacXX texture we have:
    for (unsigned long num = 0; num < total_billboards; num++) {
        vec3 rnd = world.getRandomPos();
        b.color = Colors::Silver;
        float x = rnd.x;
        //if (x >= world.getWorldSize().x / 2.0f) x = world.getWorldSize().x / 2.0f - 0.01f;
        float z = rnd.z;
        //if (z >= world.getWorldSize().z / 2.0f) z = world.getWorldSize().z / 2.0f - 0.01f;

        b.start.x = x;
        b.start.z = z;
        b.end.x = x;
        b.end.z = z;
        float h = world.getHeightmapValue(x, z);
        b.start.y = h;
        b.end.y = h + 0.25f;
        lines.push_back(b);
    }
}

void Incoming::init() {
    bool debugObjects = true; // false to disable all helper objects
    float aspectRatio = engine.getAspect();

    // 2 square km world size
    //world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    world.setWorldSize(1024.0f, 382.0f, 1024.0f);
    engine.setWorld(&world);

    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_2m.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    //engine.meshStore.loadMesh("terrain2k/Project_Mesh_0.5.gltf", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.meshStore.loadMesh("incoming/valley_Mesh_0.5.glb", "WorldBaseTerrain", MeshFlagsCollection(MeshFlags::MESH_TYPE_NO_TEXTURES));
    //engine.meshStore.loadMesh("incoming/flat.glb", "WorldBaseTerrain", MeshType::MESH_TYPE_NO_TEXTURES);
    engine.objectStore.createGroup("terrain_group");
    if (debugObjects) {
        engine.objectStore.createGroup("knife_group");
        engine.objectStore.createGroup("box_group");
        engine.meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
        engine.meshStore.loadMesh("box1_cmp.glb", "Box1");
        engine.meshStore.loadMesh("box10_cmp.glb", "Box10");
        engine.meshStore.loadMesh("box100_cmp.glb", "Box100");
        engine.meshStore.loadMesh("bottle2.glb", "WaterBottle");
    }

    auto terrain = engine.objectStore.addObject("terrain_group", "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));
    engine.objectStore.createGroup("weapon_group");
    engine.meshStore.loadMesh("cyberpunk_pistol_cmp.glb", "Gun", MeshFlagsCollection(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER));
    gun = engine.objectStore.addObject("weapon_group", "Gun", vec3(4.97f, 57.39f, -3.9));
    gun->scale() = vec3(0.03f, 0.03f, 0.03f);
    //gun->rot() = vec3(4.8, 6.4, 7.4);
    gun->rot() = vec3(0.1, 0.1, 0.1);
    worldObject = gun;
    if (debugObjects) {
        //auto knife = engine.objectStore.addObject("knife_group", "Knife", vec3(900.0f, 20.0f, 0.3f));
        WorldObject* knife = nullptr;
        knife = engine.objectStore.addObject("knife_group", "Knife", vec3(5.47332f, 58.312f, 3.9));
        knife->rot().x = 3.14159f / 2;
        knife->rot().y = -3.14159f / 4;
        auto bottle = engine.objectStore.addObject("knife_group", "WaterBottle", vec3(5.77332f, 58.43f, 3.6));
        auto box1 = engine.objectStore.addObject("box_group", "Box1", vec3(5.57332f, 57.3f, 3.70005));
        auto box10 = engine.objectStore.addObject("box_group", "Box10", vec3(-5.57332f, 57.3f, 3.70005));
        auto box100 = engine.objectStore.addObject("box_group", "Box100", vec3(120.57332f, 57.3f, 3.70005));
    }
    world.transformToWorld(terrain);
    auto p = hmdPositioner.getPosition();

    // heightmap
    //engine.textureStore.loadTexture("valley_height.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT,
    //    TextureFlags::KEEP_DATA_BUFFER | TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX);
    engine.textureStore.loadTexture("flat.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT,
        TextureFlags::KEEP_DATA_BUFFER | TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX);
    auto texHeightmap = engine.textureStore.getTexture("heightmap");
    world.setHeightmap(texHeightmap);
    unsigned int texIndexHeightmap = texHeightmap->index;
    //shaders.billboardShader.setHeightmapTextureIndex(texIndexHeightmap);
    //world.checkTerrainSuitableForHeightmap(terrain);
    world.prepareUltimateHeightmap(terrain);
    world.paths.init(&world, terrain, &world.ultHeightInfo);
    // switch to walking mode:
    fpPositioner.setModeWalking();
    hmdPositioner.setModeWalking();
    Log("Camera set to walking mode.\n")

    // skybox
    engine.textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    engine.shaders.cubeShader.setSkybox("skyboxTexture");
    engine.shaders.cubeShader.setFarPlane(2000.0f);


    engine.shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine.shaders.pbrShader.initialUpload();
    if (enableLines) {
        // overlay terrain triangles with lines
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
        addRandomHeightLines(grid->lines, world);
        // draw one line of heightmap data:
        for (int i = 0; i < world.getWorldSize().x*2; i++) {
            float v = -512.0f + (i * 0.5f);
            float w = v + 0.5f;
            vec3 p1 = vec3(v, 0.0f, v);
            //vec3 p2 = vec3(w, 0.0f, w);
            vec3 p2 = vec3(v, 0.0f, v);
            p1.y = world.getHeightmapValueWC(v, v);// +0.25f;
            //p2.y = world.getHeightmapValue(w, w) + 0.25f;
            p2.y = world.getHeightmapValueWC(v, v) + 0.25f;
            grid->lines.push_back(LineDef(p1, p2, vec4(1.0f, 1.0f, 1.0f, 1.0f)));
            //auto bug = world.getHeightmapValue(19.0f, 19.0f);
            //Log("bug: " << bug << endl);
        }
        engine.shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine.shaders.lineShader.uploadFixedGlobalLines();
    }
    // load and play music
    engine.sound.openSoundFile("inc_background.ogg", "BACKGROUND_MUSIC", true);
    engine.sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 0.8f, 100);
    // load sound effects
    engine.sound.openSoundFile("lock_and_load_big_gun.ogg", "LOAD_GUN");
    engine.sound.openSoundFile("announce_under_attack.ogg", "ANNOUNCE_UNDER_ATTACK");
    engine.sound.openSoundFile("single_gun_shot_two.ogg", "SHOOT_GUN");

    // add sound to object
    if (enableSound) {
        engine.sound.addWorldObject(gun);
        //engine.sound.changeSound(gun, "BACKGROUND_MUSIC");
        //engine.sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
    }

    game.addGamePhase(PhaseIntro, "Intro");
    game.addGamePhase(PhasePrepare, "Pickup Weapon");
    game.addGamePhase(PhasePhase1, "Phase One");
    game.addGamePhase(PhasePhase2, "Phase Two");
    game.addGamePhase(PhasePhase3, "Phase Three");
    game.addGamePhase(PhaseEnd, "The End");
    game.addGamePhase(PhaseEndTitles, "Titles");

    game.setPhase(PhasePrepare);

    // start with holding weapon
    holdWeapon = true;
    game.setPhase(PhasePhase1);

    if (game.isGamePhase(PhasePrepare)) {
        engine.sound.playSound("ANNOUNCE_UNDER_ATTACK", SoundCategory::EFFECT, 200.0f, 4000);
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
    old_seconds = seconds;

    updateCameraPositioners(deltaSeconds);
    //if (tr.frameNum % 100 == 0) camera->log();

    // lines
    if (enableLines) {
        LineShader::UniformBufferObject lubo{};
        LineShader::UniformBufferObject lubo2{};
        lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
        applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
        engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
        engine.shaders.lineShader.clearLocalLines(tr);
        if (enableIntersectTest) {
            vector<LineDef> oneTimelines;
            oneTimelines.push_back(intersectTestLine);
            engine.shaders.lineShader.addOneTime(oneTimelines, tr);
        }
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
    vector<LineDef> boundingBoxes;
    static LineDef shootLine;
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

        } else {
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
            if (enableLines) {
                if (enableIntersectTest) {
                    vec3 axes[3];
                    wo->calculateBoundingBoxWorld(modeltransform);
                    if (wo->isLineIntersectingBoundingBox(intersectTestLine.start, intersectTestLine.end, axes)) {
                        if (wo->mesh->id.starts_with("Water")) {
                            Util::drawBoxFromAxes(boundingBoxes, axes);
                        }
                        Log("Line intersects bounding box of " << wo->mesh->id << endl);
                        wo->drawBoundingBox(boundingBoxes, modeltransform, Colors::Red);
                    }
                } else {
                    wo->drawBoundingBox(boundingBoxes, modeltransform);
                    if (wo->isLineIntersectingBoundingBox(shootLine.start, shootLine.end)) {
                        Log("Line intersects bounding box of " << wo->mesh->id << endl);
                    }
                }
            }
        }
        // move gun to fixed position in camera space
        if (wo->mesh->id.starts_with("Gun") && holdWeapon) {
            auto* positioner = getFirstPersonCameraPositioner();
            vec3 deltaPos(0.2f, -0.28f, 0.5f);
            Movement mv;
            vec3 finalPos;
            modeltransform = positioner->moveObjectToCameraSpace(wo, deltaPos, r, &finalPos, &mv);
            if (activePositionerIsHMD) {
                // should update gun independently from camera, will be fixed until we have hand position from VR
                auto* positioner = getHMDCameraPositioner();
                modeltransform = positioner->moveObjectToCameraSpace(wo, deltaPos, r, &finalPos, &mv);
            }
            // draw lines for up, right and forward vectors
            if (enableLines && false) {
                vec3 pos = finalPos;
                vector<LineDef> oneTimelines;
                LineDef l;
                l.color = Colors::Red;
                l.start = pos;
                l.end = pos + mv.right;
                oneTimelines.push_back(l);
                l.color = Colors::Blue;
                l.end = pos + mv.up;
                oneTimelines.push_back(l);
                l.color = Colors::Green;
                l.end = pos + mv.forward;
                oneTimelines.push_back(l);
                shootLine = l;
                engine.shaders.lineShader.addOneTime(oneTimelines, tr);
            }
        }
        buf->model = modeltransform;
    }
    if (enableLines) {
        engine.shaders.lineShader.addOneTime(boundingBoxes, tr);
        engine.shaders.lineShader.prepareAddLines(tr);
    }
    // game Logic
    if (game.isGamePhase(PhasePrepare)) {
        // pickup weapon - check distance
        float dist = gun->distanceTo(camera->getPosition());
        //Log("Distance to weapon: " << dist << endl);
        if ( dist < pickupDistance) {
            Log("Picked up weapon" << endl);
            holdWeapon = true;
            game.setPhase(PhasePhase1);
            engine.sound.playSound("LOAD_GUN", SoundCategory::EFFECT, 300.0f);
            //engine.sound.changeSound(gun, "LOAD_GUN");
            //engine.sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
        }
    }
    if (processGunshot) {
        processGunshot = false;
        if (game.isGamePhase(PhasePhase1)) {
            //Log("Shot weapon" << endl);
            engine.sound.playSound("SHOOT_GUN", SoundCategory::EFFECT, 300.0f);
        }
    }


    postUpdatePerFrame(tr);
}

void Incoming::handleInput(InputState& inputState)
{
    auto key = inputState.key;
    auto action = inputState.action;
    auto mods = inputState.mods;
    // disable F key
    if (inputState.keyEvent) {
        if (key == GLFW_KEY_F && action == GLFW_RELEASE) {
            return;
        }
    }
    AppSupport::handleInput(inputState);
    if (inputState.mouseButtonEvent && inputState.pressedLeft) {
        //Log("SHOOT!" << endl);
        processGunshot = true;
    }
    if (inputState.keyEvent) {
        //Log("key pressed: " << inputState.key << endl);
        const bool press = action != GLFW_RELEASE;
        float alterRadians = 0.1 / 4.0;
        bool shift = mods & GLFW_MOD_SHIFT;
        //handleKeyInputTurnWeapon(shift, mods, key);
        handleKeyInputIntersectTest(shift, mods, key);
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

void Incoming::handleKeyInputTurnWeapon(bool shift, int mods, int key) {
    float alterRadians = 0.1 / 4.0;
    if (key == GLFW_KEY_X) {
        if (shift) {
            r.x -= alterRadians;
        }
        else {
            r.x += alterRadians;
        }
        Log("rot x " << r.x << endl);
    }
    if (key == GLFW_KEY_Y) {
        if (shift) {
            r.y -= alterRadians;
        }
        else {
            r.y += alterRadians;
        }
        Log("rot y " << r.y << endl);
    }
    if (key == GLFW_KEY_Z) {
        if (shift) {
            r.z -= alterRadians;
        }
        else {
            r.z += alterRadians;
        }
        Log("rot z " << r.z << endl);
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

void Incoming::handleKeyInputIntersectTest(bool shift, int mods, int key) {
    float step = 0.1 / 4.0;
    vec3 pt = intersectTestModifyStartPoint ? intersectTestLine.start : intersectTestLine.end;
    if (key == GLFW_KEY_X) {
        if (shift) {
            pt.x -= step;
        }
        else {
            pt.x += step;
        }
        Log("pt x " << pt.x << endl);
    }
    if (key == GLFW_KEY_Y) {
        if (shift) {
            pt.y -= step;
        }
        else {
            pt.y += step;
        }
        Log("pt y " << pt.y << endl);
    }
    if (key == GLFW_KEY_Z) {
        if (shift) {
            pt.z -= step;
        }
        else {
            pt.z += step;
        }
        Log("pt z " << pt.z << endl);
    }
    if (key == GLFW_KEY_D) {
    }
    if (key == GLFW_KEY_1) {
        intersectTestModifyStartPoint = true;
        Log("Modify start point" << endl);
    }
    if (key == GLFW_KEY_2) {
        intersectTestModifyStartPoint = false;
        Log("Modify end point" << endl);
    }
    if (mods & GLFW_MOD_SHIFT) {
    }
    if (key == GLFW_KEY_SPACE) {
    }

    if (intersectTestModifyStartPoint) {
        intersectTestLine.start = pt;
    } else {
        intersectTestLine.end = pt;
    }
}

