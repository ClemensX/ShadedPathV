#include "pch.h"

using namespace std;
using namespace glm;

void LandscapeGenerator::run()
{
    Log("LandscapeGenerator started" << endl);
    {
        // camera initialization camera pos (241.638,732.069,2261.37) look at (-0.108512,-0.289912,-0.950882)
        CameraPositioner_FirstPerson positioner(glm::vec3(241.638f, 732.069f, 2261.37f), glm::vec3(-0.108512f, -0.289912f, -0.950882f), glm::vec3(0.0f, 1.0f, 0.0f));
        positioner.setMaxSpeed(25.0f);
        Camera camera(positioner);
        this->camera = &camera;
        this->positioner = &positioner;
        engine.enableKeyEvents();
        engine.enableMousButtonEvents();
        engine.enableMouseMoveEvents();
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
        engine.enablePresentation(win_width, (int)(win_width / 1.77f), "Landscape Generator (Diamond Square Algorithm)");
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 4300.0f));

        engine.setFramesInFlight(2);
        engine.registerApp(this);
        //engine.enableSound();
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init("LandscapeGenerator");

        engine.textureStore.generateBRDFLUT();
        //this_thread::sleep_for(chrono::milliseconds(3000));
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

        // some shaders may need additional preparation
        engine.prepareDrawing();


        // rendering
        while (!engine.shouldClose()) {
            engine.pollEvents();
            engine.drawFrame();
        }
        engine.waitUntilShutdown();
    }
    Log("LandscapeGenerator ended" << endl);
}

void LandscapeGenerator::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    //world.setWorldSize(10.0f, 382.0f, 10.0f);

    // add some lines:
    float aspectRatio = engine.getAspect();

    engine.shaders.lineShader.initialUpload();
}

void LandscapeGenerator::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine.shaders.submitFrame(tr);
}

void LandscapeGenerator::updatePerFrame(ThreadResources& tr)
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
    engine.shaders.lineShader.clearAddLines(tr);
    {
        // thread protection needed
        std::unique_lock<std::mutex> lock(monitorMutex);
        if (parameters.generate && parameters.n > 0) {
            parameters.generate = false;
            //Log("Generate thread " << tr.frameIndex << endl);
            int n2plus1 = (int)(pow(2, parameters.n) + 1);
            Spatial2D heightmap(n2plus1);
            int lastPos = n2plus1 - 1;
            // down left and right corner
            heightmap.setHeight(0, 0, parameters.h_bl);
            heightmap.setHeight(lastPos, 0, parameters.h_br);
            // top left and right corner
            heightmap.setHeight(0, lastPos, parameters.h_tl);
            heightmap.setHeight(lastPos, lastPos, parameters.h_tr);
            // do two iteration
            heightmap.diamondSquare(parameters.magnitude, parameters.dampening, parameters.seed, parameters.generations);
            lines.clear();
            heightmap.getLines(lines);
            heightmap.adaptLinesToWorld(lines, world);
            //vector<vec3> plist;
            //heightmap.getPoints(plist);
            //Log("num points: " << plist.size() << endl);
        }
        engine.shaders.lineShader.addOneTime(lines, tr);
    }

    LineShader::UniformBufferObject lubo{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    //if (tr.frameNum % 100 == 0) camera->log();
    lubo.view = camera->getViewMatrix();
    lubo.proj = camera->getProjectionNDC();
    // we still need to call prepareAddLines() even if we didn't actually add some
    engine.shaders.lineShader.prepareAddLines(tr);

    // TODO hack 2nd view
    mat4 v2 = translate(lubo.view, vec3(0.3f, 0.0f, 0.0f));
    auto lubo2 = lubo;
    lubo2.view = v2;

    engine.shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
}

void LandscapeGenerator::handleInput(InputState& inputState)
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
        const char* kname = glfwGetKeyName(inputState.key, inputState.scancode);
        if (kname != NULL) {
            //Log("key == " << key << " " << kname << endl);
            if (strcmp("+", kname) == 0 && action == GLFW_RELEASE) {
                parameters.paramsChangedOutsideUI = true;
                parameters.generations += 1;
            }
            if (strcmp("-", kname) == 0 && action == GLFW_RELEASE) {
                parameters.paramsChangedOutsideUI = true;
                parameters.generations -= 1;
            }
            if (strcmp("g", kname) == 0 && action == GLFW_RELEASE) {
                parameters.paramsChangedOutsideUI = true;
                parameters.seed += 1;
            }
        }
    }
}

static void HelpMarker(const char* desc)
{
    //ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void LandscapeGenerator::buildCustomUI()
{
    static string helpText =
        "g generate new seed\n" 
        "+ next Generation\n"
        "- previous Generation";
    static Parameters localp = initialParameters;
    if (parameters.paramsChangedOutsideUI) {
        parameters.paramsChangedOutsideUI = false;
        parameters.generate = true;
        localp = parameters;
    }
    if (ImGui::CollapsingHeader("Params"))
    {
        ImGui::SameLine(); HelpMarker(helpText.c_str());
        ImGui::Separator();
        ImGui::Text("Diamond Square Parameters");
        ImGui::PushItemWidth(120);
        ImGui::InputInt("N", &localp.n);
        ImGui::SameLine();
        int n2plus1 = 0;
        if (2 < localp.n && localp.n < 14) {
            n2plus1 = (int)(pow(2, localp.n) + 1);
        }
        else {
            n2plus1 = 0;
        }

        ImGui::Text("pixel width: %d", n2plus1);
        ImGui::SameLine();
        ImGui::InputFloat("Dampening", &localp.dampening, 0.01f, 0.1f, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Magnitude", &localp.magnitude, 1.00f, 10.0f, "%.1f");
        //seed = parameters.seed;
        ImGui::SameLine();
        ImGui::InputInt("Seed", &localp.seed, 1, 10);

        ImGui::Text("Start hight for corners:");
        ImGui::SameLine();
        ImGui::InputFloat("Top L", &localp.h_tl, 1.0f, 10.0f, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Top R", &localp.h_tr, 1.0f, 10.0f, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Bottom L", &localp.h_bl, 1.0f, 10.0f, "%.3f");
        ImGui::SameLine();
        ImGui::InputFloat("Buttom R", &localp.h_br, 1.0f, 10.0f, "%.3f");

        ImGui::InputInt("Iterations", &localp.generations);
        ImGui::SameLine();
        static int clicked = 0;
        if (ImGui::Button("Generate")) {
            clicked++;
        }
        if (clicked & 1)
        {
            // generate new landscape
            parameters.n = localp.n;
            parameters.dampening = localp.dampening;
            parameters.magnitude = localp.magnitude;
            parameters.seed = localp.seed;
            parameters.h_tl = localp.h_tl;
            parameters.h_tr = localp.h_tr;
            parameters.h_bl = localp.h_bl;
            parameters.h_br = localp.h_br;
            parameters.generations = localp.generations;
            parameters.generate = true;
            clicked = 0;
        }

        ImGui::PopItemWidth();
    }
    ImGui::SameLine(); HelpMarker(helpText.c_str());
};
