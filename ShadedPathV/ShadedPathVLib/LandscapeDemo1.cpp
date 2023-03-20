#include "pch.h"

using namespace std;
using namespace glm;

void LandscapeDemo::run()
{
    Log("LandscapeDemo started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        positioner.setMaxSpeed(5.0f);
        Camera camera(positioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        //engine.enableMeshShader();
        //engine.enableVR();
        //engine.enableStereo();
        engine.enableStereoPresentation();
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(20);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK); // 960
        int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Landscape Demo");
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 2000.0f));

        engine.setFramesInFlight(2);
        engine.registerApp(this);
        //engine.enableSound();
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init("LandscapeDemo");

        engine.textureStore.generateBRDFLUT();
        //this_thread::sleep_for(chrono::milliseconds(3000));
        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            //.addShader(shaders.cubeShader)  // enable to render central cube with debug texture
            .addShader(shaders.billboardShader)
            .addShader(shaders.lineShader)  // enable to see zero cross and billboard debug lines
            .addShader(shaders.pbrShader)
            ;
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
    Log("LandscapeDemo ended" << endl);
}

void addRandomBillboards(vector<BillboardDef>& billboards, World &world, unsigned int textureIndex, float aspectRatio) {
    BillboardDef b;
    b.pos = vec4(0.0f, 4.05f, 0.0f, 0);
    b.dir = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    b.w = b.h = 10.0f;
    b.w = b.h / aspectRatio;
    b.type = 0;
    b.textureIndex = textureIndex;
    //unsigned long total_billboards = 50000000; // close to 4GB on GPU
    //unsigned long total_billboards = 1000000;
    //unsigned long total_billboards = 500000;
    //unsigned long total_billboards = 200000;
    unsigned long total_billboards = 5000;
    //unsigned long total_billboards = 12;
    unsigned long billboards_per_texture = total_billboards / 12;

    // create randomly positioned billboards with certain min distance to origin:
    for (unsigned long num = 0; num < total_billboards; num++) {
        vec3 rnd;
        float dist;
        do {
            rnd = world.getRandomPos();
            dist = sqrt(rnd.x * rnd.x + rnd.z * rnd.z);
        } while (dist < 100.0f);
        b.pos.x = rnd.x;
        //b.pos.y = rnd.y;
        b.pos.z = rnd.z;
        billboards.push_back(b);
    }
}

void LandscapeDemo::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    //world.setWorldSize(10.0f, 382.0f, 10.0f);

    // load skybox cube texture
    //engine.textureStore.loadTexture("arches_pinetree_high.ktx2", "skyboxTexture");
    //engine.textureStore.loadTexture("arches_pinetree_low.ktx2", "skyboxTexture");
    engine.textureStore.loadTexture("debug.ktx", "2dTexture");
    engine.textureStore.loadTexture("eucalyptus.ktx2", "tree");
    engine.textureStore.loadTexture("shadedpath_logo.ktx2", "logo");
    unsigned int texIndexTree = engine.textureStore.getTexture("tree")->index;
    unsigned int texIndexLogo = engine.textureStore.getTexture("logo")->index;
    unsigned int texIndex = texIndexTree;
    // add some lines:
    float aspectRatio = engine.getAspect();

    //scale tree height to 10m
    float height = 10.0f;
    float width = height / aspectRatio;
    BillboardDef myBillboards[] = {
        { vec4(-0.1f, 4.05f, -0.1f, 1.0f), // pos
          vec4(0.0f, 0.0f, 1.0f, 0.0f), // dir
          width, // w
          height, // h
          1,    // type
          texIndex
        },
        { vec4(-0.2f, 0.2f, 0.0f, 1.0f), // pos
          vec4(0.3f, 0.1f, 0.0f, 0.0f), // dir
          0.3f, // w
          0.9f, // h
          0,    // type
          texIndex
        }
    };
    vector<BillboardDef> billboards;
    addRandomBillboards(billboards, world, texIndex, aspectRatio);

    engine.shaders.billboardShader.add(billboards);

    // Grid with 1m squares, floor on -10m, ceiling on 372m
    Grid* grid = world.createWorldGrid(1.0f, 0.0f);
    heightmap = new Spatial2D(4096*2+1);
    engine.shaders.lineShader.add(grid->lines);

    // select texture by uncommenting:
    engine.global.createCubeMapFrom2dTexture("2dTexture", "2dTextureCube");
    engine.shaders.cubeShader.setFarPlane(1.0f); // cube around center
    engine.shaders.cubeShader.setSkybox("2dTextureCube");

    engine.shaders.lineShader.initialUpload();
    //engine.shaders.pbrShader.initialUpload();
    //engine.shaders.cubeShader.initialUpload();
    engine.shaders.billboardShader.initialUpload();
}

void LandscapeDemo::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void LandscapeDemo::updatePerFrame(ThreadResources& tr)
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
    // we still need to call prepareAddLines() even if we didn't actually add some
    engine.shaders.lineShader.prepareAddLines(tr);

    // TODO hack 2nd view
    mat4 v2 = translate(lubo.view, vec3(0.3f, 0.0f, 0.0f));
    auto lubo2 = lubo;
    lubo2.view = v2;

    engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    // cube
    CubeShader::UniformBufferObject cubo{};
    cubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    cubo.view = camera->getViewMatrixAtCameraPos();
    cubo.view = lubo.view; // uncomment to have stationary cube, not centered at camera
    cubo.proj = lubo.proj;
    auto cubo2 = cubo;
    cubo2.view = cubo.view;
    engine.shaders.cubeShader.uploadToGPU(tr, cubo, cubo2, true);
 
    // billboards
    BillboardShader::UniformBufferObject bubo{};
    bubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    bubo.view = lubo.view;
    bubo.proj = lubo.proj;
    auto bubo2 = bubo;
    bubo2.view = lubo2.view;
    engine.shaders.billboardShader.uploadToGPU(tr, bubo, bubo2);
    //Util::printMatrix(bubo.proj);
}

void LandscapeDemo::handleInput(InputState& inputState)
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