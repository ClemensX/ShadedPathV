#include "mainheader.h"
#include "AppSupport.h"
#include "LineApp.h"

using namespace std;
using namespace glm;

void LineApp::run(ContinuationInfo* cont)
{
    Log("LineApp start\n");
    AppSupport::setEngine(engine);
    Shaders& shaders = engine->shaders;

    // camera initialization
    createFirstPersonCameraPositioner(glm::vec3(-0.36f, 0.0f, 2.340f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    createHMDCameraPositioner(glm::vec3(-0.38f, 0.10f, 0.82f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
    initCamera();
    enableEventsAndModes();
    engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
    engine->files.findAssetFolder("data");
    setHighBackbufferResolution();
    camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.1f, 2000.0f);

    // add shaders used in this app
    shaders
        .addShader(shaders.uiShader)
        .addShader(shaders.clearShader)
        .addShader(shaders.lineShader)
        ;
    // init shaders, e.g. one-time uploads before rendering cycle starts go here
    shaders.initActiveShaders();

    init();
    engine->eventLoop();

    // cleanup
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
        //engine->meshStore.loadMeshWireframe("WaterBottle.glb", "WaterBottle", lines);
        //auto o = engine->meshStore.getMesh("WaterBottle");
        //Log("Object loaded: " << o->id.c_str() << endl);
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
    engine->shaders.clearShader.setClearColor(vec4(0.1f, 0.1f, 0.9f, 1.0f));

    prepareWindowOutput("Line App");
    engine->presentation.startUI();
}

void LineApp::mainThreadHook()
{
}

// prepare drawing, guaranteed single thread
void LineApp::prepareFrame(FrameResources* fr)
{
    frameNum = fr->frameNum;
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
        { glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f), glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-0.25f, -0.25f * aspectRatio, 1.0f), glm::vec3(0.0f, 0.25f * aspectRatio, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) }
    };
    if (engine->isVR()) plus += 0.0001f;
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
    }
    else {
        if ((tr.frameNum + 9) % 10 == 0) {
            // global update
            //increaseLineStack(permlines);
            //engine->shaders.lineShader.addPermament(permlines, tr);
        }
    }

    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);

    //logCameraPosition();
    //this_thread::sleep_for(chrono::milliseconds(300));
    engine->shaders.clearShader.addCommandBuffers(fr, &fr->drawResults[0]); // put clear shader first
}

// draw from multiple threads
void LineApp::drawFrame(FrameResources* fr, int topic, DrawResult* drawResult)
{
    if (topic == 0) {
        // draw lines
        engine->shaders.lineShader.addCommandBuffers(fr, drawResult);
    }
}

void LineApp::postFrame(FrameResources* fr)
{
    engine->shaders.endShader.addCommandBuffers(fr, fr->getLatestCommandBufferArray());
}

void LineApp::processImage(FrameResources* fr)
{
    //Log("LineApp postFrame " << fi->frameNum << endl);
    if (false && fr->frameNum > 10 && fr->frameNum <= 12) {
        Log("dump to file frame " << fr->frameNum << " " << fr->colorImage.fba.image << endl);
        dumpToFile(fr);
    }
    if (false && fr->frameNum == 1000 ) {
        Log("dump to file frame " << fr->frameNum << " " << fr->colorImage.fba.image << endl);
        dumpToFile(fr);
    }
    if (false && fr->frameNum == 10) {
        Log("sleep...");
        this_thread::sleep_for(chrono::seconds(2));
    }
    present(fr);
}

bool LineApp::shouldClose()
{
    return shouldStopEngine;
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
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    AppSupport::handleInput(inputState);
}
