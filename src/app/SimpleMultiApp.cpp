#include "mainheader.h"
#include "SimpleMultiApp.h"

void SimpleMultiApp::prepareFrame(FrameInfo* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());

    //Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        //shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

void SimpleMultiApp::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr && !window1wasopened) {
        openWindow("Window Frame 4");
        window1wasopened = true;
    }
    if (lastFrameNum >= 8 && window2.glfw_window == nullptr && !window2wasopened) {
        window2wasopened = true;
        openAnotherWindow("Another Win 8");
        imageConsumer->setWindow(&window2);
        engine->setImageConsumer(imageConsumer);
    }
    if (lastFrameNum >= 1000 && window2.glfw_window != nullptr && window2.swapChain) {
        Log("Terminate win 2 presentation");
        engine->setImageConsumer(&imageConsumerNullify);
        window2.disabled = true;
        engine->presentation.windowInfo = &window1;
    }
    if (lastFrameNum >= 1500 && window2.glfw_window != nullptr && window2.swapChain) {
        Log("Terminate win 2 presentation");
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
void SimpleMultiApp::drawFrame(FrameInfo* fi, int topic) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 2);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleMultiApp::run() {
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

bool SimpleMultiApp::shouldClose() {
    return shouldStop;
}

void SimpleMultiApp::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
    engine->enablePresentation(&window1, win_width, (int)(win_width / 1.77f), title);
    //engine->enableWindowOutput(&window1);

}

void SimpleMultiApp::openAnotherWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 480;
    engine->enablePresentation(&window2, win_width, (int)(win_width / 1.77f), title);
    engine->enableWindowOutput(&window2);
}

void SimpleMultiApp::handleInput(InputState& inputState)
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
