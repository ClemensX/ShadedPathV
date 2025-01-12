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
    Log("TestApp started - press Q to start next application!!\n");
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

void SimpleMultiApp::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
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
        if (key == GLFW_KEY_Q && press) {
            shouldStopEngine = true;
        }
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
    engine->configureParallelAppDrawCalls(2);

    if (true) {
        if (engine->getContinuationInfo() != nullptr) {
            Log("This app should continue in same glfw window that the app before!\n");
        }
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
    return shouldStopEngine;
}

void SimpleMultiApp2::openWindow(const char* title) {
    Log("open or reuse Window " << title << std::endl);
    if (engine->getContinuationInfo() != nullptr) {
        engine->presentation.reuseWindow(&window1, engine->getContinuationInfo());
    } else {
        int win_width = 480;
        engine->presentation.createWindow(&window1, win_width, (int)(win_width / 1.77f), title);
    }
    engine->enablePresentation(&window1);
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
        shouldStopEngine = true;
    }
}
