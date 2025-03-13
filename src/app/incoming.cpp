#include "mainheader.h"
#include "AppSupport.h"
#include "incoming.h"

using namespace std;
using namespace glm;

void Incoming::run(ContinuationInfo* cont)
{
    Log("Incoming started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        // camera initialization
        vec3 camStart(5.38f, 58.7576f, 5.30f);
        initCamera(camStart, vec3(0.0f, 50.0f, -100.0f), vec3(0.0f, 1.0f, 0.0f));
        setMaxSpeed(15.0f);

        if (enableIntersectTest) {
            intersectTestLine.color = Colors::Red;
            intersectTestLine.start = camStart + vec3(0.5f, -0.3f, 0.0f);
            intersectTestLine.end = camStart + vec3(0.5f, -0.3f, -1.0f);
        }

        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        //engine->setFrameCountLimit(1000);
        setHighBackbufferResolution();
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.10f, 2000.0f);

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

void Incoming::addRandomRock(RockInfo ri, std::vector<WorldObject*>& rockList)
{
    // Generate a random number between 0 and 9
    int randomValue = rand() % 10;
    string rockName = "Rocks." + to_string(randomValue);

    WorldObject* rock = engine->objectStore.addObject(GroupRocksName, rockName, ri.pos);
    rockList.push_back(rock);
}

void Incoming::addRandomRockFormations(RockWave waveName, std::vector<WorldObject*>& rockList)
{
    // define start pos, then add rocks to left, right and on top
    vec3 pos(0.0f, 70.0f, -80.0);
    float xDist = 20.0f;
    float zDist = 20.0f;
    float yDist = 20.0f;

    // Seed the random number generator
    std::srand(314); // any value will do - but we want to have same rand every time for consistent graphics

    WorldObject* rock;
    RockInfo ri;
    int factorX, factorY, factorZ;
    if (waveName == RockWave::Cube) {
        // do 5x5x5 cube of rocks
        factorX = factorY = factorZ = 1;
        for (int x = -2; x <= 2; x++) {
            for (int y = 0; y <= 4; y++) {
                for (int z = -4; z <= 0; z++) {
                    ri.pos = pos + vec3(x * xDist * factorX, y * yDist * factorY, z * zDist * factorZ);
                    addRandomRock(ri, rockList);
                }
            }
        }   
    }
}

void Incoming::init() {
    engine->sound.init();
    bool debugObjects = false; // false to disable all helper objects
    float aspectRatio = engine->getAspect();

    // 2 square km world size
    //world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    world.setWorldSize(1024.0f, 382.0f, 1024.0f);
    engine->setWorld(&world);

    MeshFlagsCollection meshFlags = MeshFlagsCollection(MeshFlags::MESH_TYPE_NO_TEXTURES);
    meshFlags.setFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER);
    engine->meshStore.loadMesh("incoming/valley_Mesh_0.5.glb", "WorldBaseTerrain", meshFlags);
    engine->objectStore.createGroup(GroupTerrainName, GroupTerrain);
    if (debugObjects) {
        engine->objectStore.createGroup(GroupDebugName, GroupDebug);
        engine->meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
        engine->meshStore.loadMesh("box1_cmp.glb", "Box1");
        engine->meshStore.loadMesh("box10_cmp.glb", "Box10");
        engine->meshStore.loadMesh("box100_cmp.glb", "Box100");
        engine->meshStore.loadMesh("bottle2.glb", "WaterBottle");
    }

    // terrain (has to be first object in world)
    auto terrain = engine->objectStore.addObject(GroupTerrainName, "WorldBaseTerrain", vec3(0.3f, 0.0f, 0.0f));

    // rocks
    engine->objectStore.createGroup(GroupRocksName, GroupRocks);
    engine->meshStore.loadMesh("rocks_multi_cmp.glb", "Rocks");
    addRandomRockFormations(RockWave::Cube, rockObjects);

    // weapon
    engine->objectStore.createGroup(GroupGunName, GroupGun);
    engine->meshStore.loadMesh("cyberpunk_pistol_cmp.glb", "Gun", MeshFlagsCollection(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER));
    gun = engine->objectStore.addObject(GroupGunName, "Gun", vec3(4.97f, 57.39f, -3.9));
    gun->scale() = vec3(0.03f, 0.03f, 0.03f);
    //gun->rot() = vec3(4.8, 6.4, 7.4);
    gun->rot() = vec3(0.1, 0.1, 0.1);
    worldObject = gun;

    if (debugObjects) {
        WorldObject* knife = nullptr;
        knife = engine->objectStore.addObject(GroupDebugName, "Knife", vec3(5.47332f, 58.312f, 3.9));
        knife->rot().x = 3.14159f / 2;
        knife->rot().y = -3.14159f / 4;
        auto bottle = engine->objectStore.addObject(GroupDebugName, "WaterBottle", vec3(5.77332f, 58.43f, 3.6));
        auto box1 = engine->objectStore.addObject(GroupDebugName, "Box1", vec3(5.57332f, 57.3f, 3.70005));
        auto box10 = engine->objectStore.addObject(GroupDebugName, "Box10", vec3(-5.57332f, 57.3f, 3.70005));
        auto box100 = engine->objectStore.addObject(GroupDebugName, "Box100", vec3(120.57332f, 57.3f, 3.70005));
    }
    world.transformToWorld(terrain);
    auto p = hmdPositioner.getPosition();

    // heightmap
    engine->textureStore.loadTexture("flat.ktx2", "heightmap", TextureType::TEXTURE_TYPE_HEIGHT,
        TextureFlags::KEEP_DATA_BUFFER | TextureFlags::ORIENTATION_RAW_START_WITH_XMAX_ZMAX);
    auto texHeightmap = engine->textureStore.getTexture("heightmap");
    world.setHeightmap(texHeightmap);
    unsigned int texIndexHeightmap = texHeightmap->index;
    world.prepareUltimateHeightmap(terrain);
    world.paths.init(&world, terrain, &world.ultHeightInfo);
    // switch to walking mode:
    fpPositioner.setModeWalking();
    hmdPositioner.setModeWalking();
    Log("Camera set to walking mode.\n")

    // skybox
    engine->textureStore.loadTexture("cube_sky.ktx2", "skyboxTexture");
    engine->textureStore.generateBRDFLUT();
    // generating cubemaps makes shader debugPrintf failing, so we load pre-generated cubemaps
    //engine->textureStore.generateCubemaps("skyboxTexture");
    engine->textureStore.loadTexture("irradiance.ktx2", engine->textureStore.IRRADIANCE_TEXTURE_ID);
    engine->textureStore.loadTexture("prefilter.ktx2", engine->textureStore.PREFILTEREDENV_TEXTURE_ID);
    engine->shaders.cubeShader.setSkybox("skyboxTexture");
    engine->shaders.cubeShader.setFarPlane(2000.0f);


    engine->shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));
    engine->shaders.pbrShader.initialUpload();
    if (enableLines) {
        // Grid with 100m squares, floor on -10m, ceiling on 372m
        Grid* grid = world.createWorldGrid(100.0f, 0.0f);
        engine->shaders.lineShader.addFixedGlobalLines(grid->lines);
        engine->shaders.lineShader.uploadFixedGlobalLines();
    }
    // load and play music
    engine->sound.openSoundFile("inc_background.ogg", "BACKGROUND_MUSIC", true);
    engine->sound.playSound("BACKGROUND_MUSIC", SoundCategory::MUSIC, 0.8f, 100);
    // load sound effects
    engine->sound.openSoundFile("lock_and_load_big_gun.ogg", "LOAD_GUN");
    engine->sound.openSoundFile("announce_under_attack.ogg", "ANNOUNCE_UNDER_ATTACK");
    engine->sound.openSoundFile("single_gun_shot_two.ogg", "SHOOT_GUN");

    // add sound to object
    if (engine->isSoundEnabled()) {
        engine->sound.addWorldObject(gun);
        //engine->sound.changeSound(gun, "BACKGROUND_MUSIC");
        //engine->sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
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
    //holdWeapon = true;
    //game.setPhase(PhasePhase1);

    if (game.isGamePhase(PhasePrepare)) {
        engine->sound.playSound("ANNOUNCE_UNDER_ATTACK", SoundCategory::EFFECT, 200.0f, 4000);
    }
    prepareWindowOutput("Incoming");
    engine->presentation.startUI();
}

void Incoming::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void Incoming::prepareFrame(FrameResources* fr)
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
        engine->shaders.lineShader.clearLocalLines(tr);
        if (enableIntersectTest) {
            vector<LineDef> oneTimelines;
            oneTimelines.push_back(intersectTestLine);
            engine->shaders.lineShader.addOneTime(oneTimelines, tr);
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
    vector<LineDef> boundingBoxes;
    vec3 finalGunPos(1000, 1000, 1000);
    for (auto& wo : engine->objectStore.getSortedList()) {
        PBRShader::DynamicModelUBO* buf = engine->shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        if (wo->userGroupId == GroupTerrain) {
            //terrain
            modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1f, 0.0f, 0.0f));
            uiVerticesTotal = wo->mesh->vertices.size();
            uiVerticesSqrt = (unsigned long)sqrt(uiVerticesTotal);
            // transform from gltf:
            modeltransform = wo->mesh->baseTransform;

        } else {
            if (!wo->enabled) {
                // TODO OUCH! this is a hack to disable object rendering (means not seeing them)
                wo->pos().y = -30000.0f;
            }
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
            wo->calculateBoundingBoxWorld(modeltransform);
            if (enableLines) {
                if (enableIntersectTest) {
                    if (wo->isLineIntersectingBoundingBox(intersectTestLine.start, intersectTestLine.end)) {
                        Log("Line intersects bounding box of " << wo->mesh->id << endl);
                        wo->drawBoundingBox(boundingBoxes, modeltransform, Colors::Red);
                    }
                } else {
                    if (wo->drawNormals) {
                        wo->drawBoundingBox(boundingBoxes, modeltransform, Colors::Red);
                    }
                }
            }
        }
        // move gun to fixed position in camera space
        if (wo->userGroupId == GroupGun && holdWeapon) {
            auto* positioner = getFirstPersonCameraPositioner();
            vec3 deltaPos(0.2f, -0.28f, 0.5f);
            Movement mv;
            modeltransform = positioner->moveObjectToCameraSpace(wo, deltaPos, r, &finalGunPos, &mv);
            if (activePositionerIsHMD) {
                // should update gun independently from camera, will be fixed until we have hand position from VR
                auto* positioner = getHMDCameraPositioner();
                modeltransform = positioner->moveObjectToCameraSpace(wo, deltaPos, r, &finalGunPos, &mv);
            }

            // calc shoot line
            vec3 pos = finalGunPos;
            LineDef l;
            l.start = pos;
            float forwardLength = length(mv.forward);
            l.end = pos + mv.forward * (2000.0f / forwardLength);
            shootLine = l;
            if (enableLines) {
                // draw lines for up, right and forward vectors
                vector<LineDef> oneTimelines;
                l.color = Colors::Green;
                oneTimelines.push_back(l);
                l.color = Colors::Red;
                l.end = pos + mv.right;
                oneTimelines.push_back(l);
                l.color = Colors::Blue;
                l.end = pos + mv.up;
                oneTimelines.push_back(l);
                engine->shaders.lineShader.addOneTime(oneTimelines, tr);
            }
        }
        buf->model = modeltransform;
    }
    if (enableLines) {
        engine->shaders.lineShader.addOneTime(boundingBoxes, tr);
        engine->shaders.lineShader.prepareAddLines(tr);
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
            engine->sound.playSound("LOAD_GUN", SoundCategory::EFFECT, 300.0f);
            //engine->sound.changeSound(gun, "LOAD_GUN");
            //engine->sound.setSoundRolloff("BACKGROUND_MUSIC", 0.1f);
        }
    }
    if (processGunshot) {
        processGunshot = false;
        if (game.isGamePhase(PhasePhase1)) {
            //Log("Shot weapon" << endl);
            engine->sound.playSound("SHOOT_GUN", SoundCategory::EFFECT, 300.0f);
            WorldObject* nearestShotObject = nullptr;
            float lastDistance = 100000.0f;
            for (auto& wo : rockObjects) {
                if (wo-> enabled && wo->isLineIntersectingBoundingBox(shootLine.start, shootLine.end)) {
                    float distGunRock = wo->distanceTo(finalGunPos);
                    if ( distGunRock < lastDistance) {
                        lastDistance = distGunRock;
                        nearestShotObject = wo;
                    }
                }
            }
            if (nearestShotObject != nullptr) {
                //Log("rock destroyed " << wo->mesh->id << endl);
                //wo->drawBoundingBox(boundingBoxes, modeltransform, Colors::Red);
                nearestShotObject->enabled = false;
            }
        }
    }


    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void Incoming::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
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

void Incoming::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void Incoming::processImage(FrameResources* fr)
{
    present(fr);
}

bool Incoming::shouldClose()
{
    return shouldStopEngine;
}

void Incoming::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
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
    float step = 0.1 / 12.0;
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

