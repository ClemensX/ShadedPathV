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
        openWindow("Multi App Window");
        window1wasopened = true;
        imageConsumer->setWindow(&window1);
        engine->setImageConsumer(imageConsumer);
    }
    if (lastFrameNum >= 2000 && window1.glfw_window != nullptr && window1.swapChain == nullptr) {
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

void SimpleMultiApp::run(ContinuationInfo* cont) {
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
    cont->cont = true;
};

bool SimpleMultiApp::shouldClose() {
    return shouldStop;
}

void SimpleMultiApp::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 480;
    engine->enablePresentation(&window1, win_width, (int)(win_width / 1.77f), title);
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
        shouldStop = true;
    }
}

// app2 
void SimpleMultiApp2::prepareFrame(FrameInfo* fi) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());

    //Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        //shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

void SimpleMultiApp2::mainThreadHook() {
    if (lastFrameNum >= 4 && window1.glfw_window == nullptr && !window1wasopened) {
        openWindow("Multi App Window");
        window1wasopened = true;
        imageConsumer->setWindow(&window1);
        engine->setImageConsumer(imageConsumer);
    }
    if (lastFrameNum >= 2000 && window1.glfw_window != nullptr && window1.swapChain == nullptr) {
    }
}

// drawFrame is called for each topic in parallel!! Beware!
void SimpleMultiApp2::drawFrame(FrameInfo* fi, int topic) {
    if (!engine->isSingleThreadMode()) assert(false == engine->isMainThread());
    if (topic == 0) {
        //Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 2);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleMultiApp2::run(ContinuationInfo* cont) {
    Log("TestApp started\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);
    engine->configureParallelAppDrawCalls(2);

    if (cont->cont) {
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
    } else {
        // we don't want to continue with 2nd app
        Log("ERROR: app cannot continue\n");
    }

};

bool SimpleMultiApp2::shouldClose() {
    return shouldStop;
}

void SimpleMultiApp2::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 480;
    engine->enablePresentation(&window1, win_width, (int)(win_width / 1.77f), title);
    engine->enableWindowOutput(&window1);
}

void SimpleMultiApp2::handleInput(InputState& inputState)
{
    assert(engine->isMainThread());
    if (inputState.windowClosed != nullptr) {
        if (inputState.windowClosed == &window1) {
            //Log("Window 1 shouldclosed\n");
        }
        inputState.windowClosed = nullptr;
        shouldStop = true;
    }
}
