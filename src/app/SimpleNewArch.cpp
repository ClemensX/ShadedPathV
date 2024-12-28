#include "mainheader.h"
#include "SimpleNewArch.h"

void SimpleApp::prepareFrame(FrameInfo* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());

    //Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        //shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

void SimpleApp::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr) {
        openWindow("Window Frame 4");
    }
    if (lastFrameNum >= 8 && window2.glfw_window == nullptr) {
        openAnotherWindow("Another Win 8");
        imageConsumer->setWindow(&window2);
        engine->setImageConsumer(imageConsumer);
    }
}

// drawFrame is called for each topic in parallel!! Beware!
void SimpleApp::drawFrame(FrameInfo* fi, int topic) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 0);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleApp::run() {
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

bool SimpleApp::shouldClose() {
    return shouldStop;
}

void SimpleApp::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
    engine->enablePresentation(&window1, win_width, (int)(win_width / 1.77f), title);
    //engine->enableWindowOutput(&window1);

}

void SimpleApp::openAnotherWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 480;
    engine->enablePresentation(&window2, win_width, (int)(win_width / 1.77f), title);
    engine->enableWindowOutput(&window2);
}

void SimpleApp::handleInput(InputState& inputState)
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
        shouldStop = true;
    }
}
