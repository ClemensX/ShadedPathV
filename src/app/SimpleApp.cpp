#include "mainheader.h"
#include "AppSupport.h"
#include "SimpleApp.h"

using namespace std;
using namespace glm;

void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        Shaders& shaders = engine->shaders;
        // camera initialization
        createFirstPersonCameraPositioner(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        createHMDCameraPositioner(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //getFirstPersonCameraPositioner()->setMaxSpeed(15.0f);
        initCamera();
        // engine configuration
        enableEventsAndModes();
        engine->gameTime.init(GameTime::GAMEDAY_REALTIME);
        engine->files.findAssetFolder("data");
        engine->setMaxTextures(10);
        //engine->setFrameCountLimit(1000);
        //engine->setBackBufferResolution(ShadedPathEngine::Resolution::FourK);
        if (vr) {
            engine->setBackBufferResolution(ShadedPathEngine::Resolution::HMDIndex);
        } else {
            engine->setBackBufferResolution(ShadedPathEngine::Resolution::FourK); // 960
        }
        int win_width = 960; //2500;//1800;// 800;//3700;
        engine->enablePresentation(win_width, (int)(win_width /1.77f), "Vulkan Simple App");
        camera->saveProjectionParams(glm::radians(45.0f), engine->getAspect(), 0.1f, 2000.0f);

        engine->registerApp(this);
        initEngine("SimpleApp");
        engine->textureStore.generateBRDFLUT();
        // add shaders used in this app
        shaders
            .addShader(shaders.uiShader)
            .addShader(shaders.clearShader)
            .addShader(shaders.lineShader)
            .addShader(shaders.simpleShader)
            ;
        // init shaders, e.g. one-time uploads before rendering cycle starts go here
        shaders.initActiveShaders();

        // init app rendering:
        init();
        eventLoop();
    }
    Log("SimpleApp ended" << endl);
}

void SimpleApp::init() {
    // add some lines:
    float aspectRatio = engine->getAspect();
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

    engine->shaders.lineShader.addFixedGlobalLines(lines);

    // 2 square km world size
    world.setWorldSize(2048.0f, 382.0f, 2048.0f);
    // Grid with 1m squares, floor on -10m, ceiling on 372m
    Grid *grid = world.createWorldGrid(1.0f, -10.0f);
    engine->shaders.lineShader.addFixedGlobalLines(grid->lines);
    engine->shaders.lineShader.uploadFixedGlobalLines();
}

void SimpleApp::drawFrame(ThreadResources& tr) {
    updatePerFrame(tr);
    engine->shaders.submitFrame(tr);
}

void SimpleApp::updatePerFrame(ThreadResources& tr)
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
    SimpleShader::UniformBufferObject ubo{};
    SimpleShader::UniformBufferObject ubo2{};
    //float a = 0.3f; float b = 140.0f; float z = 15.0f;
    float a = 0.3f; float b = 14.0f; float z = 15.0f;
    // move object between a, a, a and b, b, b in z seconds
    float rel_time = static_cast<float>(fmod(seconds, z));
    downmode = fmod(seconds, 2 * z) > z ? true : false;
    float objectPos = (b - a) * rel_time / z;
    if (downmode) objectPos = b - objectPos;
    else objectPos = a + objectPos;
    //Log(" " << cam << " " << downmode <<  " " << rel_time << endl);
    bool moving = false;
    glm::vec3 objectPosV;
    if (moving) {
        // either moving:
        objectPosV = glm::vec3(objectPos, objectPos, objectPos);
    } else {
        // or stationary:
        objectPosV = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)((seconds * 1.0f) * glm::radians(90.0f)), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), objectPosV);
    glm::mat4 final = trans * rot; // rot * trans will circle the object around y axis

    ubo.model = final;
    ubo2.model = final;
    applyViewProjection(ubo.view, ubo.proj, ubo2.view, ubo2.proj);

    // copy ubo to GPU:
    engine->shaders.simpleShader.uploadToGPU(tr, ubo, ubo2);

    // lines
    LineShader::UniformBufferObject lubo{};
    LineShader::UniformBufferObject lubo2{};
    lubo.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    lubo2.model = glm::mat4(1.0f); // identity matrix, empty parameter list is EMPTY matrix (all 0)!!
    applyViewProjection(lubo.view, lubo.proj, lubo2.view, lubo2.proj);

    // dynamic lines:
    engine->shaders.lineShader.clearLocalLines(tr);
    float aspectRatio = engine->getAspect();
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
    engine->shaders.lineShader.addOneTime(lines, tr);

    engine->shaders.lineShader.prepareAddLines(tr);
    engine->shaders.lineShader.uploadToGPU(tr, lubo, lubo2);
}

void SimpleApp::handleInput(InputState& inputState)
{
    AppSupport::handleInput(inputState);
}