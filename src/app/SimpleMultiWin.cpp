#include "mainheader.h"
#include "SimpleMultiWin.h"

// Use this example with care. It is a basic test for using mutiple windows with one app.
// Currently produces validation warnings if a window is removed from render queue.
// It is not recommended to use this as a base for your application.

void SimpleMultiWin::prepareFrame(FrameInfo* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());

    //Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        //shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

void SimpleMultiWin::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr && !window1wasopened) {
        reuseWindow("Window Frame 4");
        window1wasopened = true;
    }
    if (lastFrameNum >= 8 && window2.glfw_window == nullptr && !window2wasopened) {
        window2wasopened = true;
        openAnotherWindow("Another Win 8");
        imageConsumer->setWindow(&window2);
        engine->setImageConsumer(imageConsumer);
    }
    if (lastFrameNum >= 1000 && window2.glfw_window != nullptr && window2.swapChain) {
        Log("Terminate win 2 Image consumer\n");
        engine->setImageConsumer(&imageConsumerNullify);
        window2.disabled = true;
        engine->presentation.windowInfo = &window1;
    }
    if (lastFrameNum >= 1500 && window2.glfw_window != nullptr && window2.swapChain) {
        Log("Terminate win 2 presentation\n");
        //engine->setImageConsumer(&imageConsumerNullify);
        engine->presentation.endPresentation(&window2);
    }
    if (lastFrameNum >= 2000 && window1.glfw_window != nullptr && window1.swapChain == nullptr) {
        engine->enableWindowOutput(&window1);
        imageConsumer->setWindow(&window1);
        engine->setImageConsumer(imageConsumer);
    }
}

// drawFrame is called for each topic in parallel!! Beware!
void SimpleMultiWin::drawFrame(FrameInfo* fi, int topic) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 2);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleMultiWin::run(ContinuationInfo* cont) {
    Log("TestApp started\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);
    engine->configureParallelAppDrawCalls(2);
    gpui = engine->createImage("Test Image");
    engine->globalRendering.createDumpImage(directImage);
    di.openForCPUWriteAccess(gpui, &directImage);
    ImageConsumerWindow icw(engine);
    imageConsumer = &icw;

    //openWindow("Window SimpleApp");

    engine->eventLoop();

    // cleanup
    di.closeCPUWriteAccess(gpui, &directImage);
    engine->globalRendering.destroyImage(&directImage);

};

bool SimpleMultiWin::shouldClose() {
    return shouldStopEngine;
}

void SimpleMultiWin::reuseWindow(const char* title) {
    Log("reuseWindow " << title << std::endl);
    int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
    engine->presentation.createWindow(&window1, win_width, (int)(win_width / 1.77f), title);
    engine->enablePresentation(&window1);
    //engine->enableWindowOutput(&window1);

}

void SimpleMultiWin::openAnotherWindow(const char* title) {
    Log("reuseWindow " << title << std::endl);
    int win_width = 480;
    engine->presentation.createWindow(&window2, win_width, (int)(win_width / 1.77f), title);
    engine->enablePresentation(&window2);
    engine->enableWindowOutput(&window2);
}

void SimpleMultiWin::handleInput(InputState& inputState)
{
    assert(engine->isMainThread());
    if (inputState.windowClosed != nullptr) {
        if (inputState.windowClosed == &window1) {
            //Log("Window 1 shouldclosed\n");
        }
        if (inputState.windowClosed == &window2) {
            //Log("Window 2 shouldclosed\n");
        }
        inputState.windowClosed = nullptr;
        shouldStopEngine = true;
    }
}

int mainSimpleMultiWin() {
    Log("ShadedPathV app\n");
    ShadedPathEngine engine;
    engine
        .setEnableLines(true)
        .setDebugWindowPosition(true)
        .setEnableUI(true)
        .setEnableSound(true)
        .setVR(false)
        //.setSingleThreadMode(true)
        .overrideCPUCores(4)
        ;


    //engine.setFixedPhysicalDeviceIndex(0);
    engine.initGlobal();
    SimpleMultiWin app;
    engine.registerApp((ShadedPathApplication*)&app);
    engine.app->run();
    return 0;
}