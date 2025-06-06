#include "mainheader.h"
#include "AppSupport.h"
#include "DeviceCoordApp.h"

using namespace std;
using namespace glm;


void DeviceCoordApp::run(ContinuationInfo* cont)
{
    Log("DeviceCoordApp started" << endl);
    {
        AppSupport::setEngine(engine);
        auto& shaders = engine->shaders;
        // camera initialization
        positioner_.init(engine, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        positioner = &positioner_;
        Camera camera;
        camera.setEngine(engine);
        camera.changePositioner(positioner);
        this->camera = &camera;
        engine->enableKeyEvents();
        engine->enableMousButtonEvents();
        engine->enableMouseMoveEvents();
        // engine configuration
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        engine->setBackBufferResolution(ShadedPathEngine::Resolution::OneK); //oneK == 960
        int win_width = 960;//1800;// 800;//3700;
        //engine->enablePresentation(win_width, (int)(win_width /1.77f), "Vulkan Device Coordinates");
        //engine->setFramesInFlight(2);
        engine->registerApp(this);

        // engine initialization
        //engine->init("DeviceCoordApp");

        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            .addShader(shaders.lineShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        engine->eventLoop();
    }
    Log("DeviceCoordApp ended" << endl);
}

void DeviceCoordApp::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    prepareWindowOutput("Vulkan Device Coordinates");
}

// prepare drawing, guaranteed single thread
void DeviceCoordApp::prepareFrame(FrameResources* fr)
{
    FrameResources& tr = *fr;
    double seconds = engine->gameTime.getTimeSeconds();
    if ((old_seconds > 0.0f && old_seconds == seconds) || old_seconds > seconds) {
        Error("APP TIME ERROR - should not happen");
        return;
    }
    double deltaSeconds = seconds - old_seconds;
    positioner->update(deltaSeconds, input.pos, input.pressedLeft);
    old_seconds = seconds;
    bool downmode;
    float a = -1.0f; float b = 1.0f; float z = 5.0f;
    // move float vlaue object between a and b in z seconds
    float rel_time = static_cast<float>(fmod(seconds, z));
    downmode = fmod(seconds, 2 * z) > z ? true : false;
    float floatVal = (b - a) * rel_time / z;
    floatVal = (downmode) ? b - floatVal : a + floatVal;
    //Log(" floatVal " << downmode <<  " " << floatVal << endl);

    // lines
    LineShader::UniformBufferObject lubo{};
    lubo.model = mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo.view = mat4(1.0f);
    lubo.proj = mat4(1.0f);

    // dynamic lines:
    engine->shaders.lineShader.clearLocalLines(tr);
    vector<LineDef> lines;
    // x runs from -1 to 1 from left to right
    LineDef move1 = { vec3(-1.0f, floatVal,0.0f), vec3(1.0f,floatVal,0.0f), vec4(0.0f, 1.0f, 0.0f, 1.0f) };
    // y runs from -1 to 1 from top to bottom
    LineDef move2 = { vec3(floatVal, -1.0,0.0f), vec3(floatVal,1.0,0.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f) };
    float zVal = (floatVal + 1.0f) / 2.0f;
    // z runs from 0 to 1 from eye into screen (to the back)
    LineDef move3 = { vec3(-1.0f, 0.0,zVal), vec3(1.0f,0.0,zVal), vec4(0.0f, 0.0f, 1.0f, 1.0f) };
    lines.push_back(move1);
    lines.push_back(move2);
    lines.push_back(move3);
    engine->shaders.lineShader.addOneTime(lines, tr);

    engine->shaders.lineShader.prepareAddLines(tr);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo);
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void DeviceCoordApp::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        // draw lines
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
    }
}

void DeviceCoordApp::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void DeviceCoordApp::processImage(FrameResources* fr)
{
    present(fr);
}

bool DeviceCoordApp::shouldClose()
{
    return shouldStopEngine;
}

void DeviceCoordApp::handleInput(InputState& inputState)
{
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
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