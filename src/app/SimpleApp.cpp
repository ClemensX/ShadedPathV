#include "mainheader.h"
#include "SimpleApp.h"

using namespace std;
using namespace glm;

void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Camera camera(&engine);
        camera.changePositioner(positioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        //engine.enableVR();
        //engine.enableStereoPresentation();
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine.files.findAssetFolder("data");
        engine.setMaxTextures(10);
        //engine.setFrameCountLimit(1000);
        //engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::FourK); // 960
        int win_width = 960; //2500;//1800;// 800;//3700;
        engine.enablePresentation(win_width, (int)(win_width /1.77f), "Vulkan Simple App");
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.1f, 2000.0f));

        engine.setFramesInFlight(2);
        engine.registerApp(this);
        engine.setThreadModeSingle();

        // engine initialization
        engine.init("SimpleApp");

        ShaderState state;
        //engine.shaders.pbrShader.init(engine, state);
        engine.textureStore.generateBRDFLUT();
        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            .addShader(shaders.lineShader)
            .addShader(shaders.simpleShader)
            //.addShader(shaders.pbrShader)
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
    Log("SimpleApp ended" << endl);
}

void SimpleApp::init() {
    // add some lines:
    float aspectRatio = engine.getAspect();
    float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    //LineDef myLines[] = {
    //    // start, end, color
    //    { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
    //    { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
    //    { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) }
    //};
    vector<LineDef> lines;
    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    LineShader::addZeroCross(lines);
    LineShader::addCross(lines, vec3(1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f));

    engine.shaders.lineShader.addFixedGlobalLines(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m
    Grid *grid = world.createWorldGrid(1.0f, -10.0f);
    engine.shaders.lineShader.addFixedGlobalLines(grid->lines);
    engine.shaders.lineShader.uploadFixedGlobalLines();
}

void SimpleApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void SimpleApp::updatePerFrame(ThreadResources& tr)
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
    SimpleShader::UniformBufferObject ubo{};
    static bool downmode;
    //float a = 0.3f; float b = 140.0f; float z = 15.0f;
    float a = 0.3f; float b = 14.0f; float z = 15.0f;
    // move object between a, a, a and b, b, b in z seconds
    float rel_time = static_cast<float>(fmod(seconds, z));
    downmode = fmod(seconds, 2 * z) > z ? true : false;
    float objectPos = (b - a) * rel_time / z;
    if (downmode) objectPos = b - objectPos;
    else objectPos = a + objectPos;
    //Log(" " << cam << " " << downmode <<  " " << rel_time << endl);
    float cpos = 14.3f;
    glm::vec3 camPos(cpos, cpos, cpos);
    // either moving:
    //glm::vec3 objectPosV(objectPos, objectPos, objectPos);
    // or stationary:
    glm::vec3 objectPosV(0.0f, 0.0f, 0.0f);

    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)((seconds * 1.0f) * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), objectPosV);
    glm::mat4 final = trans * rot; // rot * trans will circle the object around y axis

    ubo.model = final;
    ubo.view = camera->getViewMatrix();
    ubo.proj = camera->getProjectionNDC();

    // copy ubo to GPU:
    engine.shaders.simpleShader.uploadToGPU(tr, ubo);

    // lines
    LineShader::UniformBufferObject lubo{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo.view = camera->getViewMatrix();
    lubo.proj = camera->getProjectionNDC();

    // dynamic lines:
    engine.shaders.lineShader.clearLocalLines(tr);
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
    engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo);
}

void SimpleApp::handleInput(InputState& inputState)
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