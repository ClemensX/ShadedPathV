#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Camera camera(positioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
        // engine configuration
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK);
        int win_width = 800;//3700;
        engine.enablePresentation(win_width, (int)(win_width /1.77f), "Vulkan Simple App");
        engine.enableUI();
        engine.setFramesInFlight(2);
        engine.registerApp(this);
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init();

        // shader initialization
        engine.shaders.initiateShader_Triangle();
        //engine.shaders.initiateShader_BackBufferImageDump();

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

void SimpleApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.drawFrame_Triangle(tr);
}

void SimpleApp::updatePerFrame(ThreadResources& tr)
{
    static double old_seconds = 0.0f;
    //Log("time: " << engine->gameTime.getTimeSystemClock() << endl);
    //Log("game time: " << engine->gameTime.getTimeGameClock() << endl);
    //Log("game time rel: " << setprecision(27) << engine->gameTime.getTime() << endl);
    //Log("time delta: " << setprecision(27) << engine->gameTime.getTimeDelta() << endl);
    //Log("time rel in s: " << setprecision(27) << engine->gameTime.getTimeSeconds() << endl);
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
    float rel_time = fmod(seconds, z);
    downmode = fmod(seconds, 2 * z) > z ? true : false;
    float objectPos = (b - a) * rel_time / z;
    if (downmode) objectPos = b - objectPos;
    else objectPos = a + objectPos;
    //Log(" " << cam << " " << downmode <<  " " << rel_time << endl);
    float cpos = 14.3f;
    glm::vec3 camPos(cpos, cpos, cpos);
    //glm::vec3 objectPosV(objectPos, objectPos, objectPos);
    glm::vec3 objectPosV(0.0f, 0.0f, 0.0f);

    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)((seconds * 1.0f) * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), objectPosV);
    glm::mat4 final = trans * rot; // rot * trans will circle the object around y axis

    ubo.model = final;
    ubo.view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), engine.getAspect(), 0.1f, 2000.0f);
    // flip y:
    ubo.proj[1][1] *= -1;

    ubo.view = camera->getViewMatrix();

    // copy ubo to GPU:
    engine.shaders.simpleShader.uploadToGPU(tr, ubo);
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