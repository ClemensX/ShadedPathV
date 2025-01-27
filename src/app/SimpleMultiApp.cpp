#include "mainheader.h"
#include "SimpleMultiApp.h"

void SimpleMultiApp::prepareFrame(FrameResources* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    lastFrameNum = fi->frameNum;
};

void SimpleMultiApp::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr && !window1wasopened) {
        createWindow("Multi App Window");
        window1wasopened = true;
        imageConsumer->setWindow(&window1);
        engine->setImageConsumer(imageConsumer);
    }
}

// drawFrame is called for each topic in parallel!! Beware!
void SimpleMultiApp::drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 2);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleMultiApp::run(ContinuationInfo* cont) {
    Log("WARNING: TestApp started - press Q to start next application!!\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);
    gpui = engine->createImage("Test Image");
    engine->globalRendering.createDumpImage(directImage);
    di.openForCPUWriteAccess(gpui, &directImage);
    ImageConsumerWindow icw(engine);
    imageConsumer = &icw;

    engine->shaders.initActiveShaders();
    engine->eventLoop();

    // cleanup
    di.closeCPUWriteAccess(gpui, &directImage);
    engine->globalRendering.destroyImage(&directImage);
    if (shouldStopAllApplications) {
        cont->cont = false;
    } else {
        cont->cont = true;
        engine->presentation.detachFromWindow(&window1, cont);
    }
};

bool SimpleMultiApp::shouldClose() {
    return shouldStopEngine;
}

void SimpleMultiApp::createWindow(const char* title) {
    Log("reuseWindow " << title << std::endl);
    int win_width = 480;
    engine->presentation.createWindow(&window1, win_width, (int)(win_width / 1.77f), title);
    engine->enablePresentation(&window1);
    engine->enableWindowOutput(&window1);
}

void SimpleMultiApp::handleInput(InputState& inputState)
{
    assert(engine->isMainThread());
    if (inputState.windowClosed != nullptr) {
        if (inputState.windowClosed == &window1) {
            //Log("Window 1 shouldclosed\n");
        }
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
        shouldStopAllApplications = true; // using window close to stop app chaining
    }
    if (inputState.keyEvent) {
        //Log("key pressed: " << inputState.key << endl);
        auto key = inputState.key;
        auto action = inputState.action;
        auto mods = inputState.mods;
        const bool press = action != GLFW_RELEASE;
        if (key == GLFW_KEY_Q && !press) {
            shouldStopEngine = true;
        }
    }
}

// app2 
void SimpleMultiApp2::prepareFrame(FrameResources* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    lastFrameNum = fi->frameNum;
};

void SimpleMultiApp2::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr && !window1wasopened) {
        reuseWindow("Multi App Window");
        window1wasopened = true;
        imageConsumer->setWindow(&window1);
        engine->setImageConsumer(imageConsumer);
    }
}

// drawFrame is called for each topic in parallel!! Beware!
void SimpleMultiApp2::drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 1);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleMultiApp2::run(ContinuationInfo* cont) {
    Log("TestApp started\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);

    if (engine->getContinuationInfo() == nullptr) {
        Error("No existing window found. This app should continue in same glfw window that the app before!\n");
    }
    gpui = engine->createImage("Test Image");
    engine->globalRendering.createDumpImage(directImage);
    di.openForCPUWriteAccess(gpui, &directImage);
    ImageConsumerWindow icw(engine);
    imageConsumer = &icw;

    engine->shaders.initActiveShaders();
    engine->eventLoop();

    // cleanup
    di.closeCPUWriteAccess(gpui, &directImage);
    engine->globalRendering.destroyImage(&directImage);
};

bool SimpleMultiApp2::shouldClose() {
    return shouldStopEngine;
}

void SimpleMultiApp2::reuseWindow(const char* title) {
    Log("reuse Window " << title << std::endl);
    if (engine->getContinuationInfo() == nullptr) {
        Error("No continuation info found. Reusing existing window not possible.\n");
    }
    engine->presentation.reuseWindow(&window1, engine->getContinuationInfo());
    engine->enablePresentation(&window1);
    engine->enableWindowOutput(&window1);
}

void SimpleMultiApp2::handleInput(InputState& inputState)
{
    assert(engine->isMainThread());
    if (inputState.windowClosed != nullptr) {
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
    if (inputState.keyEvent) {
        Log("key pressed: " << inputState.key << std::endl);
        auto key = inputState.key;
        auto action = inputState.action;
        auto mods = inputState.mods;
        const bool press = action != GLFW_RELEASE;
        if (key == GLFW_KEY_Q && !press) {
            shouldStopEngine = true;
        }
    }
}

// define main method here, to prevent ShadedPathV.cpp from being complex
int mainSimpleMultiApp()
{
    Log("WARNING: multi app example, press q to exit first app and start the next. Window close will end everything!\n");
    ShadedPathEngine engine;
    engine
        .setEnableLines(true)
        .setDebugWindowPosition(true)
        .setEnableUI(true)
        .setEnableSound(true)
        .setVR(false)
        //.setSingleThreadMode(true)
        .overrideCPUCores(4)
        .configureParallelAppDrawCalls(2)
        ;


    //engine.setFixedPhysicalDeviceIndex(0);
    engine.initGlobal();
    SimpleMultiApp app;
    ContinuationInfo cont;
    engine.registerApp((ShadedPathApplication*)&app);
    engine.app->run(&cont);
    if (cont.cont) {
        SimpleMultiApp2 app2;
        ShadedPathEngine engine;
        engine
            .setContinuationInfo(&cont)
            .setEnableLines(true)
            .setDebugWindowPosition(true)
            .setEnableUI(true)
            .setEnableSound(true)
            .setVR(false)
            //.setSingleThreadMode(true)
            .overrideCPUCores(4)
            .configureParallelAppDrawCalls(2)
            ;
        engine.initGlobal();
        engine.registerApp((ShadedPathApplication*)&app2);
        
        cont.cont = false; // uncomment for chained app execution:
        engine.app->run(&cont);

    }
    return 0;
}
