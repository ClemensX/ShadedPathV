#include "pch.h"


void gltfObjectsApp::run()
{
    Log("App started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f, 0.0f, 0.3f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        positioner.setMaxSpeed(0.1f);
        Camera camera(positioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        //engine.enableVR();
        engine.enableStereo();
        engine.enableStereoPresentation();
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK); // 960
        int win_width = 960;//480;// 960;//1800;// 800;//3700;
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Render glTF objects");
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.1f, 2000.0f));

        engine.setFramesInFlight(2);
        engine.registerApp(this);
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init("gltfObjects");

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.pbrShader)
            ;
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
    Log("LineApp ended" << endl);
}

void gltfObjectsApp::init() {
    // add some lines:
    float aspectRatio = engine.getAspect();
    float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    vector<LineDef> lines;

    // loading objects wireframe:
    //engine.objectStore.loadObject("WaterBottle.glb", "WaterBottle", lines);
    //engine.objectStore.loadMeshWireframe("small_knife_dagger/scene.gltf", "Knife", lines);

    // loading objects:
    //engine.meshStore.loadMesh("WaterBottle.glb", "WaterBottle");
    engine.meshStore.loadMesh("bottle2.glb", "WaterBottle");
    engine.meshStore.loadMesh("small_knife_dagger2/scene.gltf", "Knife");
    //auto o = engine.meshStore.getMesh("Knife");
    // add bottle and knife to the scene:
    engine.objectStore.createGroup("bottle_group");
    engine.objectStore.addObject("bottle_group", "WaterBottle", vec3(0.0f, 0.0f, 0.0f));
    engine.objectStore.createGroup("knife_group");
    engine.objectStore.addObject("knife_group", "Knife", vec3(0.3f, 0.0f, 0.0f));
    //engine.objectStore.addObject("knife_group", "WaterBottle", vec3(0.3f, 0.0f, 0.0f));
    //Log("Object loaded: " << o->id.c_str() << endl);


    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    LineShader::addZeroCross(lines);
    //LineShader::addCross(lines, vec3(1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f));

    engine.shaders.lineShader.add(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

    engine.shaders.lineShader.initialUpload();
    engine.shaders.pbrShader.initialUpload();
}

void gltfObjectsApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void gltfObjectsApp::updatePerFrame(ThreadResources& tr)
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
    LineShader::UniformBufferObject lubo{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo.view = camera->getViewMatrix();
    lubo.proj = camera->getProjectionNDC();

    // TODO hack 2nd view
    mat4 v2 = translate(lubo.view, vec3(0.3f, 0.0f, 0.0f));
    auto lubo2 = lubo;
    lubo2.view = v2;

    // dynamic lines:
    engine.shaders.lineShader.clearAddLines(tr);
    float aspectRatio = engine.getAspect();
    static float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) }
    };
    plus += 0.001f;
    vector<LineDef> lines;
    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    engine.shaders.lineShader.addOneTime(lines, tr);

    engine.shaders.lineShader.prepareAddLines(tr);
    engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    // pbr
    PBRShader::UniformBufferObject pubo{};
    mat4 modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    pubo.model = modeltransform;
    pubo.view = lubo.view;
    pubo.proj = lubo.proj;
    auto pubo2 = pubo;
    pubo2.view = lubo2.view;
    engine.shaders.pbrShader.uploadToGPU(tr, pubo, pubo2);
    // change individual objects position:
    //auto grp = engine.objectStore.getGroup("knife_group");
    for (auto& wo : engine.objectStore.getSortedList()) {
        //Log(" adapt object " << obj.get()->objectNum << endl);
        //WorldObject *wo = obj.get();
        PBRShader::DynamicUniformBufferObject* buf = engine.shaders.pbrShader.getAccessToModel(tr, wo->objectNum);
        mat4 modeltransform;
        bool moveObjects = false;
        if (moveObjects) {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 10.0f), 0.0f, 0.0f));
            }
            else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + (plus / 100.0f), 0.0f, 0.0f));
            }
        } else {
            if (wo->objectNum == 0) {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.1f, 0.0f, 0.0f));
            }
            else {
                modeltransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 0.0f, 0.0f));
            }
        }
        buf->model = modeltransform;
        void* data = buf;
        //Log("APP per frame dynamic buffer to address: " << hex << data << endl);
    }
}

void gltfObjectsApp::handleInput(InputState& inputState)
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