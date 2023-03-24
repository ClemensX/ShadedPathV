#include "pch.h"

using namespace std;
using namespace glm;

void LandscapeGenerator::run()
{
    Log("LandscapeGenerator started" << endl);
    {
        // camera initialization
        CameraPositioner_FirstPerson positioner(glm::vec3(0.0f, 0.0f, 1.2f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
        camera.saveProjection(perspective(glm::radians(45.0f), engine.getAspect(), 0.01f, 2000.0f));

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

    // Grid with 1m squares, floor on -10m, ceiling on 372m
    //Grid* grid = world.createWorldGrid(1.0f, 0.0f);
    //engine.shaders.lineShader.add(grid->lines);
    Spatial2D heightmap(1024 + 1);
    // set height of three points at center
    //heightmap.setHeight(4096, 4096, 0.0f);
    //heightmap.setHeight(4095, 4096, 0.2f);
    //heightmap.setHeight(4096, 4095, 0.3f);
    int lastPos = 1024;
    // down left and right corner
    heightmap.setHeight(0, 0, 0.0f);
    heightmap.setHeight(lastPos, 0, 300.0f);
    // top left and right corner
    heightmap.setHeight(0, lastPos, 10.0f);
    heightmap.setHeight(lastPos, lastPos, 50.0f);
    // do two iteration
    heightmap.diamondSquare(200.0f, 0.5f);
    vector<LineDef> lines;
    heightmap.getLines(lines);
    heightmap.adaptLinesToWorld(lines, world);
    //vector<vec3> plist;
    //heightmap.getPoints(plist);
    //Log("num points: " << plist.size() << endl);
    engine.shaders.lineShader.add(lines);

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
    }
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
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
    ImGui::Separator();
    ImGui::Text("Diamond Square Parameters");
    ImGui::PushItemWidth(120);
    static int N = 5;
    ImGui::InputInt("N", &N);
    //ImGui::SameLine(); HelpMarker(
    //    "You can apply arithmetic operators +,*,/ on numerical values.\n"
    //    "  e.g. [ 100 ], input \'*2\', result becomes [ 200 ]\n"
    //    "Use +- to subtract.");
    ImGui::SameLine();
    int n2plus1 = 0;
    if (2 < N && N < 14) {
        n2plus1 = (int)(pow(2, N) + 1);
    } else {
        n2plus1 = 0;
    }
    
    ImGui::Text("pixel width: %d", n2plus1);
    static float dampening = 0.6f;
    ImGui::SameLine();
    ImGui::InputFloat("Dampening", &dampening, 0.01f, 0.1f, "%.3f");
    static float magnitude = 200.0f;
    ImGui::SameLine();
    ImGui::InputFloat("Magnitude", &magnitude, 1.00f, 10.0f, "%.1f");
    static float seed = 1.0f;
    ImGui::SameLine();
    ImGui::InputFloat("Seed", &seed, 0.01f, 0.1f, "%.3f");

    ImGui::PopItemWidth();
};
