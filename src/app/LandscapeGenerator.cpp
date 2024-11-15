#include "mainheader.h"
#include "AppSupport.h"
#include "LandscapeGenerator.h"

using namespace std;
using namespace glm;

void LandscapeGenerator::run()
{
    Log("LandscapeGenerator started" << endl);
    {
        auto& shaders = engine->shaders;
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(241.638f, 732.069f, 2261.37f), glm::vec3(-0.108512f, -0.289912f, -0.950882f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(241.638f, 732.069f, 2261.37f), glm::vec3(-0.108512f, -0.289912f, -0.950882f), glm::vec3(0.0f, 1.0f, 0.0f));
        CameraPositioner_AutoMove autoMovePositioner(glm::vec3(1500.0f, 700.069f, 500.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);
        this->autoMovePositioner = &autoMovePositioner;
        getFirstPersonCameraPositioner()->setMaxSpeed(25.0f);
        initCamera();
        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        engine->setMaxTextures(20);
        setHighBackbufferResolution();
        int win_width = 800;//480;// 960;//1800;// 800;//3700; // 2500
        engine->enablePresentation(win_width, (int)(win_width / 1.77f), "Landscape Generator (Diamond Square Algorithm)");
        //camera.saveProjection(perspective(glm::radians(45.0f), engine->getAspect(), 0.01f, 4300.0f));
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.01f, 4300.0f);

        engine->registerApp(this);
        initEngine("LandscapeGenerator");

        engine->textureStore.generateBRDFLUT();

        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            .addShader(shaders.lineShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();
        shaders.initiateShader_BackBufferImageDump(false); // enable image dumps upon request

        // init app rendering:
        init();
        eventLoop();
    }
    Log("LandscapeGenerator ended" << endl);
}

void LandscapeGenerator::init() {
    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    //world.setWorldSize(10.0f, 382.0f, 10.0f);

    // add some lines:
    float aspectRatio = engine->getAspect();

    //engine->shaders.lineShader.initialUpload();
}

void LandscapeGenerator::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine->shaders.submitFrame(tr);
}

void LandscapeGenerator::updatePerFrame(ThreadResources& tr)
{
    double seconds = engine->gameTime.getTimeSeconds();
    if (old_seconds > 0.0f && old_seconds == seconds) {
        Log("DOUBLE TIME" << endl);
        return;
    }
    if (old_seconds > seconds) {
        Log("INVERTED TIME" << endl);
        return;
    }
    double deltaSeconds = seconds - old_seconds;
    if (useAutoCamera) {
		autoMovePositioner->update(deltaSeconds);
	} else {
        updateCameraPositioners(deltaSeconds);
	}
    old_seconds = seconds;

    // lines
    //engine->shaders.lineShader.clearLocalLines(tr);
    {
        // thread protection needed
        if (parameters.generate && parameters.n > 0) {
            std::unique_lock<std::mutex> lock(monitorMutex);
            parameters.generate = false;
            //Log("Generate thread " << tr.frameIndex << endl);
            int n2plus1 = (int)(pow(2, parameters.n) + 1);
            heightmap.resetSize(n2plus1);
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
            //engine->shaders.lineShader.updateGlobal(lines);
            //vector<vec3> plist;
            //heightmap.getPoints(plist);
            //Log("num points: " << plist.size() << endl);
            engine->shaders.lineShader.addPermament(lines, tr);
        }
    }

    //LineShader::UniformBufferObject lubo{};
    //lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    ////if (tr.frameNum % 100 == 0) camera->log();
    //lubo.view = camera->getViewMatrix();
    //lubo.proj = camera->getProjectionNDC();
    //// we still need to call prepareAddLines() even if we didn't actually add some
    //engine->shaders.lineShader.prepareAddLines(tr);

    //// TODO hack 2nd view
    //mat4 v2 = translate(lubo.view, vec3(0.3f, 0.0f, 0.0f));
    //auto lubo2 = lubo;
    //lubo2.view = v2;

    //engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);
    engine->shaders.lineShader.prepareAddLines(tr);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
}

void LandscapeGenerator::handleInput(InputState& inputState)
{
    if (useAutoCamera != useAutoCameraCheckbox) {
        // switch camera type
        useAutoCamera = useAutoCameraCheckbox;
        if (useAutoCamera) {
            camera->changePositioner(autoMovePositioner);
        } else {
            if (vr) {
                camera->changePositioner(getHMDCameraPositioner());
            } else {
                camera->changePositioner(getFirstPersonCameraPositioner());
            }
        }
    }
    auto fpPositioner = getFirstPersonCameraPositioner();
    auto hmdPositioner = getHMDCameraPositioner();
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
        if (key == GLFW_KEY_H && press)
			writeHeightmapToRawFile();
        if (key == GLFW_KEY_P && press)
			engine->shaders.backBufferImageDumpNextFrame();
        if (key == GLFW_KEY_W) {
            fpPositioner->movement.forward_ = press;
            hmdPositioner->movement.forward_ = press;
            autoMovePositioner->movement.up_ = press;
        }
        if (key == GLFW_KEY_S) {
            fpPositioner->movement.backward_ = press;
            hmdPositioner->movement.backward_ = press;
            autoMovePositioner->movement.down_ = press;
        }
        if (key == GLFW_KEY_A) {
            fpPositioner->movement.left_ = press;
            hmdPositioner->movement.left_ = press;
        }
        if (key == GLFW_KEY_D) {
            fpPositioner->movement.right_ = press;
            hmdPositioner->movement.right_ = press;
        }
        if (key == GLFW_KEY_1) {
            fpPositioner->movement.up_ = press;
            hmdPositioner->movement.up_ = press;
            autoMovePositioner->movement.up_ = press;
        }
        if (key == GLFW_KEY_2) {
            fpPositioner->movement.down_ = press;
            hmdPositioner->movement.down_ = press;
            autoMovePositioner->movement.down_ = press;
        }
        if (mods & GLFW_MOD_SHIFT) {
            fpPositioner->movement.fastSpeed_ = press;
            hmdPositioner->movement.fastSpeed_ = press;
        }
        if (key == GLFW_KEY_SPACE)
            fpPositioner->setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
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
    if (parameters.paramsChangedOutsideUI) {
        parameters.paramsChangedOutsideUI = false;
        parameters.generate = true;
        localp = parameters;
    }
    if (!vr) {
        ImGui::Separator();
        ImGui::Text("Line count: %d", lines.size());
        ImGui::Separator();
        double time = ThemedTimer::getInstance()->getLatestTiming(TIMER_PART_GLOBAL_UPDATE);
        ImGui::Text("Last GPU Upload Time: %.1f ms", time / 1000.0f);
        ImGui::Separator();
        ImGui::Checkbox("Auto Moving Camera", &useAutoCameraCheckbox);
    }
    if (ImGui::CollapsingHeader("Params"))
    {
        if (!vr) {
            ImGui::SameLine(); HelpMarker(helpText.c_str());
            ImGui::Separator();
            ImGui::Text("Diamond Square Parameters");
            ImGui::PushItemWidth(120);
            ImGui::InputInt("N", &localp.n);
            ImGui::SameLine();
        }
        int n2plus1 = 0;
        if (2 < localp.n && localp.n < 14) {
            n2plus1 = (int)(pow(2, localp.n) + 1);
        }
        else {
            n2plus1 = 0;
        }

        if (!vr) {
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
        }
        int clicked = 0;
        if (ImGui::Button("Generate")) {
            clicked++;
            Log("button clicked " << clicked << endl);
        }
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
        if (clicked & 1)
        {
            Log("clicked " << clicked << endl);
            parameters.generate = true;
            clicked = 0;
        }

        ImGui::PopItemWidth();
    }
    ImGui::SameLine(); HelpMarker(helpText.c_str());
};

void LandscapeGenerator::writeHeightmapToRawFile()
{
    vector<glm::vec3> points;
    heightmap.getPoints(points);
    engine->util.writeHeightmapRaw(points);
}