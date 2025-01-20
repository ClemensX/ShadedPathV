#include "mainheader.h"
#include "AppSupport.h"
#include "LineApp.h"

using namespace std;
using namespace glm;

void LineApp::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void LineApp::prepareFrame(FrameInfo* fi)
{
}

// draw from multiple threads
void LineApp::drawFrame(FrameInfo* fi, int topic, DrawResult* drawResult)
{
    if (fi->frameNum % 1000 == 0 && topic == 1) {
        Log("LineApp drawFrame " << fi->frameNum << endl);
        if (fi->frameNum == 10000) {
            shouldStopEngine = true;
        }
    }
}

void LineApp::run(ContinuationInfo* cont)
{
    Log("LineApp start\n");
    Shaders& shaders = engine->shaders;
    engine->eventLoop();

    // cleanup
}

bool LineApp::shouldClose()
{
    return shouldStopEngine;
}

void LineApp::handleInput(InputState& inputState)
{
}

/*void LineApp::run()
{
    Log("SimpleApp started" << endl);
    {
        Shaders& shaders = engine->shaders;
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(-0.36f, 0.0f, 2.340f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(-0.38f, 0.10f, 0.82f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
        initCamera();
        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        //engine->setFrameCountLimit(1000);
        engine->setBackBufferResolution(ShadedPathEngine::Resolution::HMDIndex);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::OneK); // 960
        int win_width = 960;// 960;//1800;// 800;//3700;
        engine->enablePresentation(win_width, (int)(win_width / 1.77f), "Vulkan Simple Line App");
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.1f, 2000.0f);

        engine->registerApp(this);
        initEngine("LineApp");

        // add shaders used in this app
        shaders
            .addShader(shaders.clearShader)
            .addShader(shaders.lineShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        eventLoop();
    }
    Log("LineApp ended" << endl);
}

void LineApp::init() {
    // add some lines:
    float aspectRatio = engine->getAspect();
    float plus = 0.0f;
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) }
    };
    vector<LineDef> lines;

    // loading objects
    // TODO currently wireframe rendering is bugged
    if (false) {
        engine->meshStore.loadMeshWireframe("WaterBottle.glb", "WaterBottle", lines);
        auto o = engine->meshStore.getMesh("WaterBottle");
        Log("Object loaded: " << o->id.c_str() << endl);
    }


    // add all intializer objects to vector:
    for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });
    //LineShader::addZeroCross(lines);
    //LineShader::addCross(lines, vec3(1.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f));

    engine->shaders.lineShader.addFixedGlobalLines(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m

	engine->shaders.lineShader.uploadFixedGlobalLines();
}

void LineApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine->shaders.submitFrame(tr);
}

void LineApp::updatePerFrame(ThreadResources& tr)
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
    updateCameraPositioners(deltaSeconds);
    old_seconds = seconds;

    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);

    // dynamic lines:
    float aspectRatio = engine->getAspect();
    LineDef myLines[] = {
        // start, end, color
        { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f + plus), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f ), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) }
    };
    if (vr) plus += 0.0001f;
    else plus += 0.00001f;
    //plus = 0.021f;
    vector<LineDef> oneTimelines;
    // add all intializer objects to vector:
    engine->shaders.lineShader.clearLocalLines(tr);
    for_each(begin(myLines), end(myLines), [&oneTimelines](LineDef l) {oneTimelines.push_back(l); });
    engine->shaders.lineShader.addOneTime(oneTimelines, tr);
    engine->shaders.lineShader.prepareAddLines(tr);

    vector<LineDef> permlines;
    bool doSingleUpdate = false; // debug: enable single permanent update
    if (doSingleUpdate) {
        if (tr.frameNum == 11) {
            // single update
            increaseLineStack(permlines);
            engine->shaders.lineShader.addPermament(permlines, tr);
        }
        if (tr.frameNum == 100) {
            // single update
            increaseLineStack(permlines);
            engine->shaders.lineShader.addPermament(permlines, tr);
        }
        if (tr.frameNum == 200) {
            // single update
            increaseLineStack(permlines);
            engine->shaders.lineShader.addPermament(permlines, tr);
        }
    } else {
        if ((tr.frameNum + 9) % 10 == 0) {
            // global update
            increaseLineStack(permlines);
            engine->shaders.lineShader.addPermament(permlines, tr);
        }
    }

    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
    
    //logCameraPosition();
    //this_thread::sleep_for(chrono::milliseconds(300));
}

void LineApp::increaseLineStack(std::vector<LineDef>& lines)
{
    currentLineStackCount++;
    auto col = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < currentLineStackCount; i++) {
        float fac = 0.001f * engine->getAspect() * i;
        auto a1 = glm::vec3(-0.25f, fac, -0.25f);
        auto a2 = glm::vec3( 0.25f, fac, -0.25f);
        auto a3 = glm::vec3( 0.25f, fac,  0.25f);
        auto a4 = glm::vec3(-0.25f, fac,  0.25f);
        LineDef ld;
        ld.color = col;
        ld.start = a1; ld.end = a2;
        lines.push_back(ld);
        ld.start = a2; ld.end = a3;
        lines.push_back(ld);
        ld.start = a3; ld.end = a4;
        lines.push_back(ld);
        ld.start = a4; ld.end = a1;
        lines.push_back(ld);
    }
	//Log("Line stack increased to " << currentLineStackCount << endl);
}

void LineApp::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
}
*/